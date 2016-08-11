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
LIBS:pspice
LIBS:sim_test-cache
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
$Comp
L VSOURCE V1
U 1 1 57336052
P 4400 4050
F 0 "V1" H 4528 4096 50  0000 L CNN
F 1 "5V" H 4528 4005 50  0000 L CNN
F 2 "" H 4400 4050 50  0000 C CNN
F 3 "" H 4400 4050 50  0000 C CNN
F 4 "Value" H 4400 4050 60  0001 C CNN "Fieldname"
	1    4400 4050
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR1
U 1 1 573360D3
P 4400 4350
F 0 "#PWR1" H 4400 4100 50  0001 C CNN
F 1 "GND" H 4405 4177 50  0000 C CNN
F 2 "" H 4400 4350 50  0000 C CNN
F 3 "" H 4400 4350 50  0000 C CNN
	1    4400 4350
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 4350 4400 4250
Wire Wire Line
	4400 4300 5750 4300
Connection ~ 4400 4300
$Comp
L R R1
U 1 1 573360F5
P 4650 3700
F 0 "R1" V 4443 3700 50  0000 C CNN
F 1 "10k" V 4534 3700 50  0000 C CNN
F 2 "" V 4580 3700 50  0000 C CNN
F 3 "" H 4650 3700 50  0000 C CNN
	1    4650 3700
	0    1    1    0   
$EndComp
Wire Wire Line
	4500 3700 4400 3700
Wire Wire Line
	4400 3700 4400 3850
Wire Wire Line
	4800 3700 4950 3700
$Comp
L D D1
U 1 1 573361B8
P 5100 3700
F 0 "D1" H 5100 3485 50  0000 C CNN
F 1 "1N4148" H 5100 3576 50  0000 C CNN
F 2 "" H 5100 3700 50  0000 C CNN
F 3 "" H 5100 3700 50  0000 C CNN
F 4 "D" H 5100 3700 60  0001 C CNN "Spice_Primitive"
F 5 "1n914" H 5100 3700 60  0001 C CNN "Spice_Model"
F 6 "Y" H 5100 3700 60  0001 C CNN "Spice_Netlist_Enabled"
F 7 "/home/orson/workspace/kicad/demos/sim/diodes.lib" H 5100 3700 60  0001 C CNN "Spice_Lib_File"
	1    5100 3700
	-1   0    0    1   
$EndComp
$Comp
L C C1
U 1 1 5733628F
P 5400 4000
F 0 "C1" H 5515 4046 50  0000 L CNN
F 1 "100n" H 5515 3955 50  0000 L CNN
F 2 "" H 5438 3850 50  0000 C CNN
F 3 "" H 5400 4000 50  0000 C CNN
	1    5400 4000
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 573362F7
P 5750 4000
F 0 "R2" H 5680 3954 50  0000 R CNN
F 1 "1Meg" H 5680 4045 50  0000 R CNN
F 2 "" V 5680 4000 50  0000 C CNN
F 3 "" H 5750 4000 50  0000 C CNN
	1    5750 4000
	-1   0    0    1   
$EndComp
Wire Wire Line
	5250 3700 5750 3700
Wire Wire Line
	5750 3700 5750 3850
Wire Wire Line
	5400 3850 5400 3700
Connection ~ 5400 3700
Wire Wire Line
	5400 4300 5400 4150
Wire Wire Line
	5750 4300 5750 4150
Connection ~ 5400 4300
Text Notes 4300 3150 0    60   ~ 0
.op
$Comp
L +12V #PWR?
U 1 1 5787B2A9
P 6650 3300
F 0 "#PWR?" H 6650 3150 50  0001 C CNN
F 1 "+12V" H 6665 3473 50  0000 C CNN
F 2 "" H 6650 3300 50  0000 C CNN
F 3 "" H 6650 3300 50  0000 C CNN
	1    6650 3300
	1    0    0    -1  
$EndComp
$EndSCHEMATC
