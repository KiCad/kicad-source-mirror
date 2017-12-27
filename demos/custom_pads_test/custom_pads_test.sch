EESchema Schematic File Version 4
LIBS:custom_pads_test-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L device:Jumper SW1
U 1 1 56A7CE3B
P 4800 3950
F 0 "SW1" V 4900 4100 50  0000 C CNN
F 1 "R" V 4750 4100 50  0000 C CNN
F 2 "" V 4730 3950 30  0000 C CNN
F 3 "" H 4800 3950 30  0000 C CNN
	1    4800 3950
	0    -1   1    0   
$EndComp
$Comp
L custom_pads_schlib:R R2
U 1 1 56A7CEC8
P 5300 3950
F 0 "R2" V 5380 3950 50  0000 C CNN
F 1 "R" V 5300 3950 50  0000 C CNN
F 2 "" V 5230 3950 30  0000 C CNN
F 3 "" H 5300 3950 30  0000 C CNN
	1    5300 3950
	1    0    0    -1  
$EndComp
Text Label 5000 3650 0    60   ~ 0
PAD1
Text Label 5000 4300 0    60   ~ 0
PAD2
$Comp
L custom_pads_schlib:Antenna AE1
U 1 1 5A3A4E22
P 3650 2600
F 0 "AE1" H 3575 2675 50  0000 R CNN
F 1 "Antenna" H 3575 2600 50  0000 R CNN
F 2 "" H 3650 2600 50  0001 C CNN
F 3 "" H 3650 2600 50  0001 C CNN
	1    3650 2600
	1    0    0    -1  
$EndComp
$Comp
L custom_pads_schlib:Antenna AE2
U 1 1 5A3A4EF2
P 4450 2600
F 0 "AE2" H 4375 2675 50  0000 R CNN
F 1 "Antenna" H 4375 2600 50  0000 R CNN
F 2 "" H 4450 2600 50  0001 C CNN
F 3 "" H 4450 2600 50  0001 C CNN
	1    4450 2600
	1    0    0    -1  
$EndComp
$Comp
L custom_pads_schlib:GND #PWR01
U 1 1 5A3A4F3D
P 3650 2800
F 0 "#PWR01" H 3650 2550 50  0001 C CNN
F 1 "GND" H 3650 2650 50  0000 C CNN
F 2 "" H 3650 2800 50  0001 C CNN
F 3 "" H 3650 2800 50  0001 C CNN
	1    3650 2800
	1    0    0    -1  
$EndComp
$Comp
L custom_pads_schlib:GND #PWR02
U 1 1 5A3A4F67
P 4450 2800
F 0 "#PWR02" H 4450 2550 50  0001 C CNN
F 1 "GND" H 4450 2650 50  0000 C CNN
F 2 "" H 4450 2800 50  0001 C CNN
F 3 "" H 4450 2800 50  0001 C CNN
	1    4450 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	5300 4100 5300 4300
Wire Wire Line
	4800 4300 4800 4250
Wire Wire Line
	4800 4300 5300 4300
Wire Wire Line
	4800 3650 5300 3650
Wire Wire Line
	5300 3650 5300 3800
$EndSCHEMATC
