EESchema Schematic File Version 2
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
LIBS:test_pads_inside_pads-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "19 dec 2011"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	4150 1750 3750 1750
Wire Wire Line
	4150 1450 3750 1450
Text Label 3850 1450 0    60   ~ 0
NET1
$Comp
L CONN_1 P4
U 1 1 4EE5056D
P 4300 1450
F 0 "P4" H 4380 1450 40  0000 L CNN
F 1 "CONN_1" H 4300 1505 30  0001 C CNN
F 2 "Connect:1pin" H 4300 1400 60  0000 C CNN
F 3 "" H 4300 1450 60  0001 C CNN
	1    4300 1450
	1    0    0    -1  
$EndComp
$Comp
L CONN_1 P3
U 1 1 4EE5056C
P 3600 1450
F 0 "P3" H 3680 1450 40  0000 L CNN
F 1 "CONN_1" H 3600 1505 30  0001 C CNN
F 2 "Connect:1pin" H 3600 1500 60  0000 C CNN
F 3 "" H 3600 1450 60  0001 C CNN
	1    3600 1450
	-1   0    0    1   
$EndComp
$Comp
L CONN_1 P1
U 1 1 4EDF7CC5
P 3600 1750
F 0 "P1" H 3680 1750 40  0000 L CNN
F 1 "CONN_1" H 3600 1805 30  0001 C CNN
F 2 "Connect:1pin" H 3600 1800 60  0000 C CNN
F 3 "" H 3600 1750 60  0001 C CNN
	1    3600 1750
	-1   0    0    1   
$EndComp
$Comp
L CONN_1 P2
U 1 1 4EDF7CC0
P 4300 1750
F 0 "P2" H 4380 1750 40  0000 L CNN
F 1 "CONN_1" H 4300 1805 30  0001 C CNN
F 2 "Connect:1pin" H 4300 1700 60  0000 C CNN
F 3 "" H 4300 1750 60  0001 C CNN
	1    4300 1750
	1    0    0    -1  
$EndComp
Text Label 3850 1750 0    60   ~ 0
NET2
$EndSCHEMATC
