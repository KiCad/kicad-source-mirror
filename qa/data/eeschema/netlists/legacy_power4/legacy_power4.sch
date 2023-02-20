EESchema Schematic File Version 2
LIBS:symbols
LIBS:legacy_power4-cache
EELAYER 25 0
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
Text Notes 2850 1650 0    60   ~ 0
Actually VCC, see hidden pin.
$Comp
L +3.3V #PWR01
U 1 1 63F628AD
P 3100 1900
F 0 "#PWR01" H 3100 1750 50  0001 C CNN
F 1 "+3.3V" H 3100 2040 50  0000 C CNN
F 2 "" H 3100 1900 50  0001 C CNN
F 3 "" H 3100 1900 50  0001 C CNN
	1    3100 1900
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR02
U 1 1 63F628BF
P 3450 1900
F 0 "#PWR02" H 3450 1750 50  0001 C CNN
F 1 "+3.3V" H 3450 2040 50  0000 C CNN
F 2 "" H 3450 1900 50  0001 C CNN
F 3 "" H 3450 1900 50  0001 C CNN
	1    3450 1900
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 63F628CC
P 3100 2050
F 0 "R1" V 3180 2050 50  0000 C CNN
F 1 "R" V 3100 2050 50  0000 C CNN
F 2 "" V 3030 2050 50  0001 C CNN
F 3 "" H 3100 2050 50  0001 C CNN
	1    3100 2050
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 63F6292B
P 3450 2050
F 0 "R2" V 3530 2050 50  0000 C CNN
F 1 "R" V 3450 2050 50  0000 C CNN
F 2 "" V 3380 2050 50  0001 C CNN
F 3 "" H 3450 2050 50  0001 C CNN
	1    3450 2050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 63F6294F
P 3100 2200
F 0 "#PWR03" H 3100 1950 50  0001 C CNN
F 1 "GND" H 3100 2050 50  0000 C CNN
F 2 "" H 3100 2200 50  0001 C CNN
F 3 "" H 3100 2200 50  0001 C CNN
	1    3100 2200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 63F62967
P 3450 2200
F 0 "#PWR04" H 3450 1950 50  0001 C CNN
F 1 "GND" H 3450 2050 50  0000 C CNN
F 2 "" H 3450 2200 50  0001 C CNN
F 3 "" H 3450 2200 50  0001 C CNN
	1    3450 2200
	1    0    0    -1  
$EndComp
$EndSCHEMATC
