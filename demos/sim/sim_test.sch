EESchema Schematic File Version 2
LIBS:74xgxx
LIBS:74xx
LIBS:ac-dc
LIBS:actel
LIBS:adc-dac
LIBS:analog_switches
LIBS:atmel
LIBS:audio
LIBS:brooktre
LIBS:cmos4000
LIBS:cmos_ieee
LIBS:conn
LIBS:contrib
LIBS:cypress
LIBS:dc-dc
LIBS:device
LIBS:digital-audio
LIBS:display
LIBS:dsp
LIBS:elec-unifil
LIBS:ftdi
LIBS:gennum
LIBS:graphic
LIBS:hc11
LIBS:intel
LIBS:interface
LIBS:ir
LIBS:linear
LIBS:logo
LIBS:memory
LIBS:microchip
LIBS:microchip_pic10mcu
LIBS:microchip_pic12mcu
LIBS:microchip_pic16mcu
LIBS:microchip_pic18mcu
LIBS:microchip_pic32mcu
LIBS:microcontrollers
LIBS:motor_drivers
LIBS:motorola
LIBS:msp430
LIBS:nordicsemi
LIBS:nxp_armmcu
LIBS:onsemi
LIBS:opto
LIBS:philips
LIBS:power
LIBS:powerint
LIBS:pspice
LIBS:references
LIBS:regul
LIBS:relays
LIBS:rfcom
LIBS:sensors
LIBS:silabs
LIBS:siliconi
LIBS:stm8
LIBS:stm32
LIBS:supertex
LIBS:switches
LIBS:texas
LIBS:transf
LIBS:transistors
LIBS:ttl_ieee
LIBS:valves
LIBS:video
LIBS:xilinx
LIBS:fmc
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
L VSOURCE V?
U 1 1 57336052
P 4400 4050
F 0 "V?" H 4528 4096 50  0000 L CNN
F 1 "DC 10" H 4528 4005 50  0000 L CNN
F 2 "" H 4400 4050 50  0000 C CNN
F 3 "" H 4400 4050 50  0000 C CNN
F 4 "Value" H 4400 4050 60  0001 C CNN "Fieldname"
F 5 "V" H 4400 4050 60  0001 C CNN "SpicePrimitive"
F 6 "1 2" H 4100 4250 60  0001 C CNN "SpicePinMapping"
	1    4400 4050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 573360D3
P 4400 4350
F 0 "#PWR?" H 4400 4100 50  0001 C CNN
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
L R R?
U 1 1 573360F5
P 4650 3700
F 0 "R?" V 4443 3700 50  0000 C CNN
F 1 "10k" V 4534 3700 50  0000 C CNN
F 2 "" V 4580 3700 50  0000 C CNN
F 3 "" H 4650 3700 50  0000 C CNN
F 4 "1 2" H 4650 3700 60  0001 C CNN "SpiceMapping"
F 5 "R" V 4650 3700 60  0001 C CNN "SpicePrimitive"
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
L D D?
U 1 1 573361B8
P 5100 3700
F 0 "D?" H 5100 3485 50  0000 C CNN
F 1 "1N4148" H 5100 3576 50  0000 C CNN
F 2 "" H 5100 3700 50  0000 C CNN
F 3 "" H 5100 3700 50  0000 C CNN
F 4 "D" H 5100 3700 60  0001 C CNN "SpicePrimitive"
F 5 "1 2" H 5100 3700 60  0001 C CNN "SpiceMapping"
	1    5100 3700
	-1   0    0    1   
$EndComp
$Comp
L C C?
U 1 1 5733628F
P 5400 4000
F 0 "C?" H 5515 4046 50  0000 L CNN
F 1 "100n" H 5515 3955 50  0000 L CNN
F 2 "" H 5438 3850 50  0000 C CNN
F 3 "" H 5400 4000 50  0000 C CNN
F 4 "C" H 5400 4000 60  0001 C CNN "SpicePrimitive"
F 5 "1 2" H 5400 4000 60  0001 C CNN "SpiceMapping"
	1    5400 4000
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 573362F7
P 5750 4000
F 0 "R?" H 5680 3954 50  0000 R CNN
F 1 "100k" H 5680 4045 50  0000 R CNN
F 2 "" V 5680 4000 50  0000 C CNN
F 3 "" H 5750 4000 50  0000 C CNN
F 4 "1 2" H 5750 4000 60  0001 C CNN "SpiceMapping"
F 5 "R" V 5750 4000 60  0001 C CNN "SpicePrimitive"
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
$EndSCHEMATC
