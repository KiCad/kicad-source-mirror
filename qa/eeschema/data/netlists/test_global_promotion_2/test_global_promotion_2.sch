EESchema Schematic File Version 5
EELAYER 33 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 3
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
Connection ~ 4550 3250
Wire Wire Line
	3800 2300 5150 2300
Wire Wire Line
	3850 3050 5000 3050
Wire Wire Line
	3850 3150 4550 3150
Wire Wire Line
	4550 3150 4550 3250
Wire Wire Line
	4550 3250 4550 4250
Wire Wire Line
	4550 3250 5300 3250
Wire Wire Line
	4550 4250 5300 4250
Wire Wire Line
	5000 3050 5000 3950
Wire Wire Line
	5000 3950 5300 3950
Wire Wire Line
	5150 2300 5150 4100
Wire Wire Line
	5150 4100 5300 4100
Wire Wire Line
	5250 2200 3800 2200
Wire Wire Line
	5250 3100 5250 2200
Wire Wire Line
	5300 2950 3850 2950
Wire Wire Line
	5300 3100 5250 3100
Text GLabel 5250 2200 2    50   Input ~ 0
LIVE
$Comp
L Connector:Conn_01x03_Male J1
U 1 1 5CCF482D
P 3600 2300
F 0 "J1" H 3600 2500 50  0000 C CNN
F 1 "Conn_01x03_Male" H 3600 2100 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 3600 2300 50  0001 C CNN
F 3 "~" H 3600 2300 50  0001 C CNN
	1    3600 2300
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J2
U 1 1 5CC1686D
P 3650 3050
F 0 "J2" H 3650 3250 50  0000 C CNN
F 1 "Conn_01x03_Male" H 3650 2850 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 3650 3050 50  0001 C CNN
F 3 "~" H 3650 3050 50  0001 C CNN
	1    3650 3050
	1    0    0    -1  
$EndComp
$Sheet
S 5300 2800 1100 700 
U 5CC15EF9
F0 "Sheet5CC15EF8" 50
F1 "subsheet.sch" 50
F2 "LIVE" I L 5300 2950 50 
F3 "NEUTRAL" I L 5300 3250 50 
F4 "LIVE_1" I L 5300 3100 50 
$EndSheet
$Sheet
S 5300 3800 1100 700 
U 5CC165F1
F0 "sheet5CC165F1" 50
F1 "subsheet.sch" 50
F2 "LIVE" I L 5300 3950 50 
F3 "NEUTRAL" I L 5300 4250 50 
F4 "LIVE_1" I L 5300 4100 50 
$EndSheet
$EndSCHEMATC
