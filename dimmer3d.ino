/*
 * D I M M E R   24.10.2018
 */
#define Taster 12 // Taster zum Dimmen ESP-12 PIN 6
#define SYNC_50HZ 13 // hier liegt Rechteckflanke an, 50Hz ESP-12 PIN 7
#define zum_optokoppler 16 //zum MOS-Fet, Länge des Pulses gleich Helligkeit ESP-12 PIN 44444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444
#define dim_max 10000  //maximaler Wert = 100%
/*durch die Phasenverschiebung des Sync-Kondensators und die Laufzeit des Optokoppler
 wird der Nulldurchgang später erkannt. Dieses wird durch die Konstante dim_versatz
 ausgeglichen. Wenn der Eingang "an_oder_abschnitt 0 ist handelt es sich um eine 
 Phasenabschnittsteuerung, ansonsten um Phasen Anschnitt.*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include <PubSubClient.h>         //MQTT Lib
boolean ta_flag=false, ta_ruhe, dim_auf=false, dim_flag=false, dim_on=false, syn_fl_alt=false, syn_fl_alt2=false, fl_pwm=HIGH, an_oder_abschnitt=false; 
int dim_min=1600; /*wert das die Lampe gerade nocht leuchtet, kann mit mqtt verändert werden*/
int ti_dimmer=0; //max 10000 Mikrosekunden = 100Hz 50Hz nach Gleichrichter
int pointer=0, /*mqtt_dimwert=0,*/ on_off_or_dim =400, testcounter=0, testcounter1=0, an_or_ab=LOW; 
int dim_versatz=3200;//Wert in us um den die Nulldurchgangserkennung verzögert ist, kann mit mqtt und vorgestelltem 'v' verändert werden
unsigned long ti_n0=0, ti_n1=0, ti_taster=0, z_test=0, sync_test=0, last_connection=0, ti_yield=0;
char dimmer[]=",\"Dimmer\":", c;
char mqtt_server[40], mqtt_port[5] = "1883", pub_mqtt_topic[44], sub1_mqtt_topic[44], inf_mqtt_topic[44], sub_mqtt_topic[44] = "MQTT TOPIC,Bsp Wozi/ESP_7EB3_Dimmer";
WiFiClient espClient;
PubSubClient client (espClient);
//***************************************************************************
void setup() {
  Serial.begin(115200);
  Serial.println(" D I M M E R  24.10.2018");
  String mac1 = WiFi.macAddress();
  EEPROM.begin(512);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);//WiFiManager
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);         //Local intialization. Once its business is done,
  WiFiManagerParameter custom_mqtt_topic("topic", "sub_mqtt topic", sub_mqtt_topic, 44);    // there is no need to keep it around
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); //reset settings - zum Testen vorderen Kommentar entfernen
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.setTimeout(180);//sets timeout until configuration portal gets turned off, useful to make it all retry or go 
  //fetches ssid and pass and tries to connect,if it does not connect it starts an access point with the specified name
  //to sleep in seconds, here  "AutoConnectAP" and goes into a blocking loop awaiting configuration
  char AP_SSID[19]="AutoConnectAP_";AP_SSID[14]=mac1[12];AP_SSID[15]=mac1[13];AP_SSID[16]=mac1[15];AP_SSID[17]=mac1[16];AP_SSID[18]='\0';
  if(!wifiManager.autoConnect(AP_SSID,"qwertzui")) { // qwertzui ist das Passwort zum AP
    delay(3000);
    ESP.reset();//reset and try again, or maybe put it to deep sleep
  } 
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(sub_mqtt_topic, custom_mqtt_topic.getValue());
  dim_versatz = EEPROM.read(89)*100 + EEPROM.read(90); //versatz des Sync-Signals zum Nulldurchgang in microSekunden
  if(EEPROM.read(91) == 'b') an_oder_abschnitt = true; //Phasenabschnitt = true, sonst false
  else an_oder_abschnitt = false;
  dim_min = EEPROM.read(92)*100 + EEPROM.read(93); //minimalen Wert für ti_dimmer holen
  if(mqtt_server[0]=='\0') {
    Serial.println("Werte aus EEPROM holen . . .");
    for (int i = 0; i < 40 ; i++)mqtt_server[i]= char(EEPROM.read(i)); 
    for (int i = 0; i < 5 ; i++) mqtt_port[i]= char(EEPROM.read(i+40));
    for (int i = 0; i < 44 ; i++) sub_mqtt_topic[i]= char(EEPROM.read(i+45));
  }
  else {  
    Serial.println("EEPROM schreiben . . .");
    for (int i = 0; i < 40 ; i++) EEPROM.write(i,char (mqtt_server[i])); //Adresse  0 - 39
    for (int i = 0; i <  5 ; i++) EEPROM.write(i+40,mqtt_port[i]);       //Adresse 40 - 44
    for (int i = 0; i < 44 ; i++) EEPROM.write(i+45,sub_mqtt_topic[i]);  //Adresse 45 - 88
    EEPROM.commit();
  } 
  //if you get here you have connected to the WiFi
  int port= int(mqtt_port[0]&0x0F)*1000 + int(mqtt_port[1]&0x0F)*100 + int(mqtt_port[2]&0x0F)*10 + int(mqtt_port[3]&0x0F);
  Serial.print("MQTT-Server: "); Serial.print(mqtt_server); Serial.print(":"); Serial.println(port);
  sprintf(pub_mqtt_topic,"stat/%s",sub_mqtt_topic);
  Serial.print("MQTT-Topic Subsribe: "); Serial.println(sub_mqtt_topic);
  sprintf(inf_mqtt_topic,"info/%s",sub_mqtt_topic);
  Serial.print("MQTT-Topic Info: "); Serial.println(inf_mqtt_topic);
  sprintf(sub1_mqtt_topic,"cmnd/%s",sub_mqtt_topic);
  Serial.print("MQTT-Topic Publish: "); Serial.println(pub_mqtt_topic);
  Serial.print("MQTT-Topic Subsribe: "); Serial.println(sub1_mqtt_topic);
  client.setServer(mqtt_server, port);
  client.setCallback(callback); //MQTT Eingangsroutine festlegen
  pinMode(Taster,INPUT_PULLUP);
  ta_ruhe = digitalRead(Taster); //Taster/Touch mit Ruhe oder Arbeitskontakt, bzw. bei Touch GND im Ruhezustand
  pinMode(an_oder_abschnitt,INPUT_PULLUP);
  pinMode(SYNC_50HZ,INPUT_PULLUP);
  pinMode(zum_optokoppler,OUTPUT);
  digitalWrite(zum_optokoppler,LOW);
  delay(10);
  ti_n0=micros();
}
//***************************************************************************
void loop() {
  loop1(); //Hintergrundaktion (WLAN) mit yield() gezielt steuern ...
}
//****************************************************************************
void loop1() { //beim verlassen des Original loop werden WLAN Aktionen ASYNCHRON ausgeführt
  while(1==1) {
    sync_und_dim();
    //client_connected_unblocking();
    //tasterabfrage();
  }
}
//****************************************************************************
void client_connected_unblocking() {
  if (!client.connected()) {
    unsigned long now=millis();
    if(now-last_connection > 5000) {
      if(reconnect()) last_connection=0;
    }
  }
  else client.loop();
}
//****************************************************************************
boolean reconnect() {
  if (client.connect("ESP8266Client")) {
      client.subscribe(sub1_mqtt_topic); // ... and resubscribe
  }
  return client.connected();
}
//***************************************************************************
void callback(char* topic, byte* payload, unsigned int length) { //Subscibe (Empfange) MQTT Nachricht
  int mqtt_dimwert=0, j=0;
  for (int i = 0; i < length; i++) Serial.print((char)payload[i]); Serial.println();
  switch (payload[0]) {
    case 'v':                                      //Wert in us in dem das SYNC-Signal verzögert ist ...
      dim_versatz = 0;                             //...wird für exakten Phasen-An-oder Abschnitt benötigt...
      while(j < length-1) {                        //...und im EEPROM gespeichert
        dim_versatz = dim_versatz * 10 + char (payload[j+1]) - 0x30;
         j++;
      }
      EEPROM.write(89,dim_versatz/100); // 1000er und 100er in EEPROM schreiben an Adresse 89
      EEPROM.write(90,dim_versatz%100); // 10er und einer in EEPROM schreiben an Adresse 90
      EEPROM.commit();
      sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"versatz");
      mqtt_test_publish(inf_mqtt_topic,long (dim_versatz));
      break;
    case 'n':                                     //aNschnitt: Dimmer soll im Phasen Anschnittmodus arbeiten
      EEPROM.write(91,'n');                 //'n' für Phasenanschnitt in EEPROM schreiben an Adresse 91
      EEPROM.commit();
      sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"phasenaNschnitt");
      mqtt_test_publish(inf_mqtt_topic,long (0));
      break;
    case 'b':                                     //aBschnitt: Dimmer soll im Phasen Abschnittmodus arbeiten
      EEPROM.write(91,'b');                 //'b' für Phasenanschnitt in EEPROM schreiben an Adresse 91
      EEPROM.commit();
      sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"phasenaBschnitt");
      mqtt_test_publish(inf_mqtt_topic,long (0));
      break;
    case 'm':                                     //Minimum: Wert bei dem die zu steuernde Lampe gerade noch leuchtet
      dim_min = 0;         
      while(j < length-1) {                      
        dim_min = dim_min * 10 + char (payload[j+1]) - 0x30;
         j++;
      }
      EEPROM.write(92,dim_min/100); // 1000er und 100er in EEPROM schreiben an Adresse 92
      EEPROM.write(93,dim_min%100); // 10er und einer in EEPROM schreiben an Adresse 93
      EEPROM.commit();
      sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"Minimum");
      mqtt_test_publish(inf_mqtt_topic,long (dim_min));
      break;
    default:                                     //Dimmer wird mit Zahl zwischen 0 und 100 eingestellt
      while(j < length) {
        mqtt_dimwert = mqtt_dimwert * 10 + char (payload[j]) - 0x30;
          //Serial.print(mqtt_dimwert);Serial.print(",  payload= "); Serial.print(payload[j]);Serial.print(",  length= "); Serial.print(length);Serial.print(",  j= "); Serial.println(j);*/
        j++;
      }
      if(mqtt_dimwert==0) dim_on=false; // Dimmer ausschalten
      else {                            // dim_wert (0-100Prozent) in ti_dimmer umrechnen
        ti_dimmer=long (mqtt_dimwert) * long (dim_max-dim_min)/100+dim_min;
        dim_on=true;
      }
      // Serial.print("mqttdimwert: "); Serial.print(mqtt_dimwert);Serial.print(",  ti_dimmer: "); Serial.println(ti_dimmer);
    break;
  }
}
//***************************************************************************
void sync_und_dim() {
    ti_n1=micros();
    if(digitalRead(SYNC_50HZ) == LOW) {
      if(syn_fl_alt == false) {
          if( (ti_n1 - sync_test) < 19700|| (ti_n1 - sync_test) > 20300 || testcounter1++ >1500) {
            sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"Microsekunden");
            mqtt_test_publish(inf_mqtt_topic,long (ti_n1-sync_test));testcounter1 =0;
          }
          sync_test = ti_n1;
          syn_fl_alt = true;
          ti_n0 = ti_n1 - dim_versatz;
      }
    }
    else {
      if(syn_fl_alt == true && ti_n1 - ti_n0 >9000) {
        syn_fl_alt = false;
        if( testcounter++ >2501) {
          sprintf(inf_mqtt_topic,"info/%s/%s",sub_mqtt_topic,"Pulslaenge");
          mqtt_test_publish(inf_mqtt_topic,long (ti_n1-sync_test));testcounter = 0;
        }
      }
    }
    int ti_n2 = int (ti_n1 - ti_n0);
    if(ti_n2 > 10000) { // Sync 2. Halbwelle
      ti_n0 += 10000; 
      int ti_n2 = int (ti_n1 - ti_n0);
      //yield();
    }
    if(an_oder_abschnitt == false) { //Phasenanschnitt
      if(ti_n2 > (10000-ti_dimmer)&& ti_n2 <10000 && dim_on == true) pwm(HIGH);
      else pwm(LOW);
    }
    else { //Phasenabschnitt
      if(ti_n2 < ti_dimmer && dim_on == true) pwm(HIGH);
      else pwm(LOW);
    }
}  
//***************************************************************************
void pwm(boolean wert) { // hier wird der Port zum MOS-FET geschaltet und die Hintergrundfunktion für WLAN freigegeben
  if(wert==HIGH) {
    if(fl_pwm==LOW) {
      fl_pwm=HIGH;
      digitalWrite(zum_optokoppler,HIGH);
      if(ti_dimmer >= 5000) zeit_fuer_sonstiges(); //yield, taster, MQTT kümmern
    }
  }
  else {
    if(fl_pwm==HIGH) {
      fl_pwm=LOW;
      digitalWrite(zum_optokoppler,LOW);
      if(ti_dimmer < 5000) zeit_fuer_sonstiges(); //yield, taster, MQTT kümmern
    }
  }
  if(ti_dimmer <= 1000 || ti_dimmer >=9850 || dim_on == false) zeit_fuer_sonstiges(); //yield, taster, MQTT kümmern
}
//***************************************************************************
void mqtt_test_publish(char* test_topic, long testzahl) {
    char dim_als_char[15];
    sprintf(dim_als_char,"%d", testzahl);
    client.publish(test_topic, dim_als_char);
}
//***************************************************************************
void mqtt_publish() {
  int dim_prozent=0;
  char dim_als_char[7];
  if(dim_on == false) {
    //Serial.println("Dimmer = 0");
  }
  else {
    dim_prozent=long (long (ti_dimmer-dim_min) *  100) / long (dim_max-dim_min);
    if( dim_prozent<0) dim_prozent=1; //kleinster Dim-Wert ist nicht aus
    //Serial.print("Dimmer in%(MQTT-Wert): "); Serial.print(dim_prozent);Serial.print("ti_dimmer: ");Serial.println(ti_dimmer); 
  }
  sprintf(dim_als_char,"%d", dim_prozent);
  /////////////////////Serial.print("sprintf_dim_als_char: ");Serial.println(dim_als_char);
  String str = String (int(dim_prozent)); str.toCharArray(dim_als_char,5);
  ////////////////Serial.print("String_dim_als_char: ");Serial.println(dim_als_char);
  //Serial.println(int(dim_prozent));
  client.publish(pub_mqtt_topic,dim_als_char); //PUBLIZIEREN
}
//***************************************************************************/
void tasterabfrage() {
  if(digitalRead(Taster)!= ta_ruhe) { // Taster/Touch gedrückt
    if(ta_flag == false) {
      ta_flag=true;
      ti_taster=millis();
    }
    else {
      unsigned long ti_taster1=millis();
      if(ti_taster1-ti_taster > on_off_or_dim) { //bei >400ms dimmen
        if(dim_auf == true) {
          ti_dimmer += 200;
          if(ti_dimmer > dim_max) ti_dimmer = dim_max;
        }
        else {
          ti_dimmer -= 200;
          if(ti_dimmer < dim_min) ti_dimmer = dim_min; 
        }
        ti_taster = ti_taster1;
        on_off_or_dim=50; //alle 50ms Wertänderung
        dim_flag = true;
        dim_on = true;
      }
    }
  }
  else {
    if(ta_flag == true) {
      if(millis() - ti_taster > 10 && dim_flag==false) { //entprellen
        if(dim_on == true) {
          dim_on=false; //Dimmer aus
          // Serial.println("Dimmer aus");
        }
        else {
          dim_on=true;
          ti_dimmer=dim_max; // Dimmer ein 100%
          dim_auf=false;
        }
      }
      ta_flag=false;
      if(dim_flag==true) {
        on_off_or_dim=400;
        if(dim_auf==true) dim_auf=false;
        else dim_auf=true;
        dim_flag=false;
      }
      mqtt_publish();
    }
  }
}
//***************************************************************************
void zeit_fuer_sonstiges() {
  if( (micros()-ti_yield) >20000) { 
    ti_yield=micros();
    yield(); // muss spätestens alle ca. 20 msec aufgerufen werden, um WLAN z betreiben
    tasterabfrage(); // Taster zum Dimmen wird abgefragt
    client_connected_unblocking();
  }
}
//***************************************************************************

