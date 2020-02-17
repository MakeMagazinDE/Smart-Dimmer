![GitHub Logo](http://www.heise.de/make/icons/make_logo.png)

Maker Media GmbH

***

# Smarter Dimmer

mit ESP8266 (ESP-12)

Bitte beachten Sie das gegenüber der im Heft abgedruckten Version leicht geänderte Schaltbild und Layout. Hinzugekommen sind R10 und R11, die für einen sicheren Start der Schaltung bei nicht angeschlossener Last sorgen.

Gerber-Files zur Platinenfertigung; Abmessungen der Platine 71 x 71 mm, 2-lagig 1,55mm FR4, Lötstopplack oben und unten, Bestückungsdruck beidseitig.

### Stückliste

	Halbleiter
	U1, U2  PC817 Optokoppler, DIL
	U3  ESP-12E oder F
	D1..D3  S1J Diode 600V/1A
	GL1 Gleichrichter 2A/400V
	Q1  IRF740
	ZD1 Zener 10V/400mW SMD MELF (Reichelt SMD ZF 10)
 
	Widerstände
	R1  VDR 360V
	R2, R8  4k7 SMD 0805
	R3, R4, R6, R9  10k SMD 0805
	R10, R11 100k SMD 1206

	Kodensatoren
	C1  220n/275V AC RM22,5 (Entstörkondensator Klasse X2, Reichelt FUNK 220n)
	C2  47µ/16V Al-Elko SMD 5mm
	C3  100µ/10V Al-Elko SMD 5mm

	Sonstiges
	L1  Ringkerndrossel 100µH/2A
	FS1 Miniatur-Sicherung 2A flink, ggf. mit Fassung (Reichelt MIK-FLINK 2,0A und PL166600)
	TR1 AC/DC-Wandler Meanwell IRM-03 3.3V
	PL1 Stiftleiste 6pol. RM 2,54
	PL2 Anschlussklemme 3pol. RM 5,08
	PL3 Stiftleiste 3pol. RM 2,54
