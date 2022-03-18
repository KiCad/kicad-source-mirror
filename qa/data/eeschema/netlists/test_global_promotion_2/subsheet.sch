EESchema Schematic File Version 5
EELAYER 33 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 3
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
Connection ~ 4500 2400
Wire Wire Line
	3800 2400 4500 2400
Wire Wire Line
	3800 2800 4500 2800
Wire Wire Line
	3800 3000 4800 3000
Wire Wire Line
	4500 2400 4500 2450
Wire Wire Line
	4500 2400 4800 2400
Wire Wire Line
	4500 2800 4500 2750
Wire Wire Line
	4800 2400 4800 2450
Wire Wire Line
	4800 3000 4800 2750
Text HLabel 3800 2400 0    50   Input ~ 0
LIVE
Text HLabel 3800 2800 0    50   Input ~ 0
NEUTRAL
Text HLabel 3800 3000 0    50   Input ~ 0
LIVE_1
$Comp
L Device:R R1
U 1 1 5CC15F6F
P 4500 2600
AR Path="/5CC15EF9/5CC15F6F" Ref="R1"  Part="1" 
AR Path="/5CC165F1/5CC15F6F" Ref="R3"  Part="1" 
F 0 "R1" V 4580 2600 50  0000 C CNN
F 1 "R" V 4500 2600 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4430 2600 50  0001 C CNN
F 3 "~" H 4500 2600 50  0001 C CNN
	1    4500 2600
	1    0    0    -1  
$EndComp
$Comp
L Device:R R2
U 1 1 5CE0545A
P 4800 2600
AR Path="/5CC15EF9/5CE0545A" Ref="R2"  Part="1" 
AR Path="/5CC165F1/5CE0545A" Ref="R4"  Part="1" 
F 0 "R2" V 4880 2600 50  0000 C CNN
F 1 "R" V 4800 2600 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4730 2600 50  0001 C CNN
F 3 "~" H 4800 2600 50  0001 C CNN
	1    4800 2600
	1    0    0    -1  
$EndComp
$EndSCHEMATC
