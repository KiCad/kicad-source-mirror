EESchema Schematic File Version 2  date 03/08/2012 23:04:32
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
EELAYER 43  0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "3 aug 2012"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_13X2 P1
U 1 1 501C45CC
P 9600 1500
F 0 "P1" H 9600 2200 60  0000 C CNN
F 1 "CONN_13X2" V 9600 1500 50  0000 C CNN
	1    9600 1500
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR01
U 1 1 501C4637
P 10100 800
F 0 "#PWR01" H 10100 890 20  0001 C CNN
F 1 "+5V" H 10100 890 30  0000 C CNN
	1    10100 800 
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR02
U 1 1 501C4646
P 9100 800
F 0 "#PWR02" H 9100 760 30  0001 C CNN
F 1 "+3.3V" H 9100 910 30  0000 C CNN
	1    9100 800 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 501C4659
P 10100 2200
F 0 "#PWR03" H 10100 2200 30  0001 C CNN
F 1 "GND" H 10100 2130 30  0001 C CNN
	1    10100 2200
	1    0    0    -1  
$EndComp
Wire Wire Line
	10000 1100 10100 1100
Wire Wire Line
	10100 1100 10100 2200
Wire Wire Line
	10000 900  10100 900 
Wire Wire Line
	10100 900  10100 800 
Wire Wire Line
	9200 900  9100 900 
Wire Wire Line
	9100 900  9100 800 
Wire Wire Line
	10000 1200 11000 1200
Wire Wire Line
	10000 1300 11000 1300
Text Label 11000 1200 2    60   ~ 0
GPIO14_(TxD)
Text Label 11000 1300 2    60   ~ 0
GPIO15_(RxD)
Wire Wire Line
	10000 1400 11000 1400
Text Label 11000 1400 2    60   ~ 0
GPIO18_(PCM_CLK)
Wire Wire Line
	10000 1600 11000 1600
Wire Wire Line
	10000 1700 11000 1700
Text Label 11000 1600 2    60   ~ 0
GPIO23
Text Label 11000 1700 2    60   ~ 0
GPIO24
Wire Wire Line
	10000 1900 11000 1900
Wire Wire Line
	10000 2000 11000 2000
Wire Wire Line
	10000 2100 11000 2100
Text Label 11000 1900 2    60   ~ 0
GPIO25
Text Label 11000 2000 2    60   ~ 0
GPIO8_(CE0)
Text Label 11000 2100 2    60   ~ 0
CPIO7_(CE1)
NoConn ~ 10000 1800
NoConn ~ 10000 1500
NoConn ~ 10000 1000
Wire Wire Line
	9200 1000 8200 1000
Wire Wire Line
	9200 1100 8200 1100
Wire Wire Line
	9200 1200 8200 1200
Wire Wire Line
	9200 1400 8200 1400
Wire Wire Line
	9200 1500 8200 1500
Wire Wire Line
	9200 1600 8200 1600
Wire Wire Line
	9200 1800 8200 1800
Wire Wire Line
	9200 1900 8200 1900
Wire Wire Line
	9200 2000 8200 2000
NoConn ~ 9200 2100
NoConn ~ 9200 1700
NoConn ~ 9200 1300
Text Label 8200 1000 0    60   ~ 0
GPIO0_(SDA)
Text Label 8200 1100 0    60   ~ 0
GPIO1_(SCL)
Text Label 8200 1200 0    60   ~ 0
GPIO4_(GPCLK0)
Text Label 8200 1400 0    60   ~ 0
GPIO17
Text Label 8200 1500 0    60   ~ 0
GPIO21_(PCM_DOUT)
Text Label 8200 1600 0    60   ~ 0
GPIO22
Text Label 8200 1800 0    60   ~ 0
GPIO10_(MOSI)
Text Label 8200 1900 0    60   ~ 0
GPIO9_(MISO)
Text Label 8200 2000 0    60   ~ 0
GPIO11_(SCKL)
$EndSCHEMATC
