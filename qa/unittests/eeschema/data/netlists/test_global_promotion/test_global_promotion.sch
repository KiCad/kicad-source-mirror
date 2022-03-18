EESchema Schematic File Version 5
EELAYER 31 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
Comment5 ""
Comment6 ""
Comment7 ""
Comment8 ""
Comment9 ""
$EndDescr
Connection ~ 3650 1700
Wire Wire Line
	3000 1700 3250 1700
Wire Wire Line
	3550 1700 3650 1700
Wire Wire Line
	3650 1700 3650 1850
Wire Wire Line
	3650 1700 3850 1700
Wire Wire Line
	3650 2150 3650 2250
Text GLabel 3000 1700 0    50   Input ~ 0
5V
Text GLabel 3850 1700 2    50   Output ~ 0
5Vanalog
$Comp
L power:GND #PWR0101
U 1 1 5E68CE9F
P 3650 2250
F 0 "#PWR0101" H 3650 2000 50  0001 C CNN
F 1 "GND" H 3655 2077 50  0000 C CNN
F 2 "" H 3650 2250 50  0001 C CNN
F 3 "" H 3650 2250 50  0001 C CNN
	1    3650 2250
	1    0    0    -1  
$EndComp
$Comp
L Device:R R1
U 1 1 5E68B6BD
P 3400 1700
F 0 "R1" V 3606 1700 50  0000 C CNN
F 1 "R" V 3515 1700 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_Power_L25.0mm_W6.4mm_P30.48mm" V 3330 1700 50  0001 C CNN
F 3 "~" H 3400 1700 50  0001 C CNN
	1    3400 1700
	0    -1   -1   0   
$EndComp
$Comp
L Device:C C1
U 1 1 5E68C688
P 3650 2000
F 0 "C1" H 3765 2045 50  0000 L CNN
F 1 "C" H 3765 1955 50  0000 L CNN
F 2 "Capacitor_THT:CP_Axial_L18.0mm_D8.0mm_P25.00mm_Horizontal" H 3688 1850 50  0001 C CNN
F 3 "~" H 3650 2000 50  0001 C CNN
	1    3650 2000
	1    0    0    -1  
$EndComp
$Sheet
S 4900 1500 450  600 
U 5E68E19B
F0 "Subcircuit" 50
F1 "Sub.sch" 50
$EndSheet
$EndSCHEMATC
