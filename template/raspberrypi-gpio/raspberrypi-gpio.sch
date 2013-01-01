EESchema Schematic File Version 2  date 15/11/2012 21:22:43
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:rpi-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "15 nov 2012"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_13X2 P1
U 1 1 50A55ABA
P 2400 1800
F 0 "P1" H 2400 2500 60  0000 C CNN
F 1 "CONN_13X2" V 2400 1800 50  0000 C CNN
	1    2400 1800
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR01
U 1 1 50A55B18
P 1900 1050
F 0 "#PWR01" H 1900 1010 30  0001 C CNN
F 1 "+3.3V" H 1900 1160 30  0000 C CNN
	1    1900 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 1050 1900 1200
Wire Wire Line
	1900 1200 2000 1200
$Comp
L +5V #PWR02
U 1 1 50A55B2E
P 2900 1050
F 0 "#PWR02" H 2900 1140 20  0001 C CNN
F 1 "+5V" H 2900 1140 30  0000 C CNN
	1    2900 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 1050 2900 1200
Wire Wire Line
	2900 1200 2800 1200
NoConn ~ 2800 1300
Wire Wire Line
	2000 1300 1250 1300
Wire Wire Line
	2000 1400 1250 1400
Text Label 1250 1300 0    60   ~ 0
GPIO0(SDA)
Text Label 1250 1400 0    60   ~ 0
GPIO1(SCL)
Wire Wire Line
	2000 1500 1250 1500
Text Label 1250 1500 0    60   ~ 0
GPIO4
NoConn ~ 2000 1600
Wire Wire Line
	2000 1700 1250 1700
Wire Wire Line
	2000 1800 1250 1800
Wire Wire Line
	2000 1900 1250 1900
Text Label 1250 1700 0    60   ~ 0
GPIO17
Text Label 1250 1800 0    60   ~ 0
GPIO21
Text Label 1250 1900 0    60   ~ 0
GPIO22
NoConn ~ 2000 2000
Wire Wire Line
	2000 2100 1250 2100
Wire Wire Line
	2000 2200 1250 2200
Wire Wire Line
	2000 2300 1250 2300
Text Label 1250 2100 0    60   ~ 0
GPIO10(MOSI)
Text Label 1250 2200 0    60   ~ 0
GPIO9(MISO)
Text Label 1250 2300 0    60   ~ 0
GPIO11(SCLK)
NoConn ~ 2000 2400
$Comp
L GND #PWR03
U 1 1 50A55C3F
P 2900 2500
F 0 "#PWR03" H 2900 2500 30  0001 C CNN
F 1 "GND" H 2900 2430 30  0001 C CNN
	1    2900 2500
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 2500 2900 1400
Wire Wire Line
	2900 1400 2800 1400
Wire Wire Line
	2800 1500 3500 1500
Wire Wire Line
	2800 1600 3500 1600
Text Label 3500 1500 2    60   ~ 0
TXD
Text Label 3500 1600 2    60   ~ 0
RXD
Wire Wire Line
	2800 1700 3500 1700
Text Label 3500 1700 2    60   ~ 0
GPIO18
NoConn ~ 2800 1800
Wire Wire Line
	2800 1900 3500 1900
Wire Wire Line
	2800 2000 3500 2000
Text Label 3500 1900 2    60   ~ 0
GPIO23
Text Label 3500 2000 2    60   ~ 0
GPIO24
NoConn ~ 2800 2100
Wire Wire Line
	2800 2200 3500 2200
Text Label 3500 2200 2    60   ~ 0
GPIO25
Wire Wire Line
	2800 2300 3500 2300
Wire Wire Line
	2800 2400 3500 2400
Text Label 3500 2300 2    60   ~ 0
GPIO8(CE0)
Text Label 3500 2400 2    60   ~ 0
GPIO7(CE1)
$EndSCHEMATC
