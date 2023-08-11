EESchema Schematic File Version 1
LIBS:power,device,conn,valves,linear,regul,74xx,cmos4000,adc-dac,memory,xilinx,special,microcontrollers,microchip,analog_switches,motorola,intel,audio,interface,digital-audio,philips,display,cypress,siliconi,contrib,.\ecc83-pp.cache
EELAYER 23  0
EELAYER END
$Descr A4 11700 8267
Sheet 1 1
Title "ECC Push-Pull"
Date "11 dec 2006"
Rev "0.1"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L GND #PWR1
U 1 1 457DBAF8
P 6950 5150
F 0 "#PWR1" H 6950 5150 30  0001 C C
F 1 "GND" H 6950 5080 30  0001 C C
	1    6950 5150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR2
U 1 1 457DBAF5
P 6700 5900
F 0 "#PWR2" H 6700 5900 30  0001 C C
F 1 "GND" H 6700 5830 30  0001 C C
	1    6700 5900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR3
U 1 1 457DBAF1
P 5250 5900
F 0 "#PWR3" H 5250 5900 30  0001 C C
F 1 "GND" H 5250 5830 30  0001 C C
	1    5250 5900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR4
U 1 1 457DBAEF
P 4850 5900
F 0 "#PWR4" H 4850 5900 30  0001 C C
F 1 "GND" H 4850 5830 30  0001 C C
	1    4850 5900
	1    0    0    -1  
$EndComp
Connection ~ 4550 5250
Wire Wire Line
	4500 5250 4550 5250
Connection ~ 5800 5050
Wire Wire Line
	5800 5050 5400 5050
Wire Wire Line
	5400 5050 5400 4950
Wire Wire Line
	5600 4950 5600 5150
Wire Wire Line
	5800 5150 5800 4950
Wire Wire Line
	6700 5400 6700 4800
Wire Wire Line
	6700 4800 6950 4800
Connection ~ 6300 4800
Wire Wire Line
	6300 4500 6300 4900
Connection ~ 6150 3900
Wire Wire Line
	6150 4500 6150 3900
Wire Wire Line
	6700 4250 6950 4250
Connection ~ 6700 3850
Wire Wire Line
	6950 3850 5750 3850
Wire Wire Line
	5750 3850 5750 4000
Wire Wire Line
	6300 4900 5950 4900
Wire Wire Line
	5950 4900 5950 4850
Wire Wire Line
	6300 4000 6300 3900
Wire Wire Line
	6300 3900 5450 3900
Wire Wire Line
	5450 3900 5450 4000
Wire Wire Line
	5250 4850 5250 5400
Wire Wire Line
	5050 4500 4850 4500
Wire Wire Line
	4850 4500 4850 5400
Connection ~ 6950 4250
Wire Wire Line
	6950 4050 6950 4350
Connection ~ 4850 4800
Wire Wire Line
	4550 4800 4850 4800
Wire Wire Line
	6950 5150 6950 5000
Wire Wire Line
	4550 5000 4550 5300
$Comp
L PWR_FLAG #FLG5
U 1 1 457DBAC0
P 4500 5250
F 0 "#FLG5" H 4500 5520 30  0001 C C
F 1 "PWR_FLAG" H 4500 5480 30  0000 C C
	1    4500 5250
	0    -1   -1   0   
$EndComp
$Comp
L CONN_2 P4
U 1 1 456A8ACC
P 5700 5500
F 0 "P4" V 5650 5500 40  0000 C C
F 1 "CONN_2" V 5750 5500 40  0000 C C
	1    5700 5500
	0    1    1    0   
$EndComp
$Comp
L ECC83_2 U1
U 1 1 454A08DD
P 5600 4500
F 0 "U1" H 5700 4000 60  0000 C C
F 1 "ECC83" H 5200 4850 60  0000 C C
	1    5600 4500
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 4549F4BE
P 6700 4050
F 0 "C1" H 6750 4150 50  0000 L C
F 1 "10uF" H 6450 4150 50  0000 L C
	1    6700 4050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR6
U 1 1 4549F4B9
P 4550 5300
F 0 "#PWR6" H 4550 5300 30  0001 C C
F 1 "GND" H 4550 5230 30  0001 C C
	1    4550 5300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR7
U 1 1 4549F4B3
P 6950 4350
F 0 "#PWR7" H 6950 4350 30  0001 C C
F 1 "GND" H 6950 4280 30  0001 C C
	1    6950 4350
	1    0    0    -1  
$EndComp
$Comp
L CONN_2 P3
U 1 1 4549F4A5
P 7300 3950
F 0 "P3" V 7250 3950 40  0000 C C
F 1 "POWER" V 7350 3950 40  0000 C C
	1    7300 3950
	1    0    0    -1  
$EndComp
$Comp
L CONN_2 P2
U 1 1 4549F46C
P 7300 4900
F 0 "P2" V 7250 4900 40  0000 C C
F 1 "OUT" V 7350 4900 40  0000 C C
	1    7300 4900
	1    0    0    -1  
$EndComp
$Comp
L CONN_2 P1
U 1 1 4549F464
P 4200 4900
F 0 "P1" V 4150 4900 40  0000 C C
F 1 "IN" V 4250 4900 40  0000 C C
	1    4200 4900
	-1   0    0    1   
$EndComp
$Comp
L C C2
U 1 1 4549F3BE
P 6500 4800
F 0 "C2" H 6550 4900 50  0000 L C
F 1 "680nF" H 6550 4700 50  0000 L C
	1    6500 4800
	0    1    1    0   
$EndComp
$Comp
L R R3
U 1 1 4549F3AD
P 6700 5650
F 0 "R3" V 6780 5650 50  0000 C C
F 1 "100K" V 6700 5650 50  0000 C C
	1    6700 5650
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 4549F3A2
P 4850 5650
F 0 "R4" V 4930 5650 50  0000 C C
F 1 "47K" V 4850 5650 50  0000 C C
	1    4850 5650
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 4549F39D
P 5250 5650
F 0 "R2" V 5330 5650 50  0000 C C
F 1 "1.5K" V 5250 5650 50  0000 C C
	1    5250 5650
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 4549F38A
P 6300 4250
F 0 "R1" V 6380 4250 50  0000 C C
F 1 "1.5K" V 6300 4250 50  0000 C C
	1    6300 4250
	1    0    0    -1  
$EndComp
$EndSCHEMATC
