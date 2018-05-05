EESchema Schematic File Version 4
LIBS:DC26Badge-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Attacker Community Badge (DC26)"
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L dc26badge-custom:14_Segment_Dual_CC DS1
U 1 1 5AE66499
P 9450 1200
F 0 "DS1" H 9750 0   60  0000 C CNN
F 1 "14_Segment_Dual_CC" H 9400 1700 60  0000 C CNN
F 2 "DC26badge:DUAL_14SEG" H 9450 1200 60  0001 C CNN
F 3 "" H 9450 1200 60  0001 C CNN
	1    9450 1200
	1    0    0    -1  
$EndComp
$Comp
L dc26badge-custom:HT16K33-28 U1
U 1 1 5AE66C17
P 5000 1550
F 0 "U1" H 5400 700 60  0000 C CNN
F 1 "HT16K33-28" H 5000 2300 60  0000 C CNN
F 2 "DC26badge:SOIC-28-300mil" H 5000 1550 60  0001 C CNN
F 3 "" H 5000 1550 60  0001 C CNN
	1    5000 1550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR01
U 1 1 5AE66C45
P 4200 950
F 0 "#PWR01" H 4200 700 50  0001 C CNN
F 1 "GND" H 4200 800 50  0000 C CNN
F 2 "" H 4200 950 50  0001 C CNN
F 3 "" H 4200 950 50  0001 C CNN
	1    4200 950 
	0    1    1    0   
$EndComp
Wire Wire Line
	4200 950  4400 950 
$Comp
L power:VDD #PWR02
U 1 1 5AE66C6A
P 5650 800
F 0 "#PWR02" H 5650 650 50  0001 C CNN
F 1 "VDD" H 5650 950 50  0000 C CNN
F 2 "" H 5650 800 50  0001 C CNN
F 3 "" H 5650 800 50  0001 C CNN
	1    5650 800 
	1    0    0    -1  
$EndComp
Wire Wire Line
	5600 950  5650 950 
Wire Wire Line
	5650 950  5650 800 
Wire Wire Line
	5600 1050 5800 1050
Wire Wire Line
	5600 1150 5800 1150
Text Label 5600 1050 0    60   ~ 0
SDA
Text Label 5600 1150 0    60   ~ 0
SCL
Wire Wire Line
	5600 1250 5800 1250
Wire Wire Line
	5600 1350 5800 1350
Wire Wire Line
	5600 1450 5800 1450
Wire Wire Line
	5600 1550 5800 1550
Wire Wire Line
	5600 1650 5800 1650
Wire Wire Line
	5600 1750 5800 1750
Wire Wire Line
	5600 1850 5800 1850
Wire Wire Line
	5600 1950 5800 1950
Wire Wire Line
	5600 2050 5800 2050
Wire Wire Line
	5600 2150 5800 2150
Wire Wire Line
	5600 2250 5800 2250
Wire Wire Line
	4400 1950 4150 1950
Wire Wire Line
	4400 2050 4150 2050
Wire Wire Line
	4400 2150 4150 2150
Wire Wire Line
	4400 2250 4150 2250
Text Label 5750 1250 2    60   ~ 0
E
Text Label 5750 1350 2    60   ~ 0
L
Text Label 5750 1450 2    60   ~ 0
N
Text Label 5750 1550 2    60   ~ 0
M
Text Label 5750 1650 2    60   ~ 0
G2
Text Label 5750 1750 2    60   ~ 0
D
Text Label 5750 1850 2    60   ~ 0
DP
Text Label 5750 1950 2    60   ~ 0
C
Text Label 5750 2050 2    60   ~ 0
G1
Text Label 5750 2150 2    60   ~ 0
A
Text Label 5750 2250 2    60   ~ 0
B
Text Label 4200 2250 0    60   ~ 0
K
Text Label 4200 2150 0    60   ~ 0
J
Text Label 4200 2050 0    60   ~ 0
H
Text Label 4200 1950 0    60   ~ 0
F
NoConn ~ 4400 1850
Wire Wire Line
	4400 1050 4150 1050
Wire Wire Line
	4400 1750 4150 1750
Wire Wire Line
	4150 1650 4400 1650
Wire Wire Line
	4400 1550 4150 1550
Wire Wire Line
	4150 1450 4400 1450
Wire Wire Line
	4400 1350 4150 1350
Wire Wire Line
	4400 1250 4150 1250
Wire Wire Line
	4150 1150 4400 1150
Text Label 4200 1050 0    60   ~ 0
DIG0
Text Label 4200 1150 0    60   ~ 0
DIG1
Text Label 4200 1250 0    60   ~ 0
DIG2
Text Label 4200 1350 0    60   ~ 0
DIG3
Text Label 4200 1450 0    60   ~ 0
DIG4
Text Label 4200 1550 0    60   ~ 0
DIG5
Text Label 4200 1650 0    60   ~ 0
DIG6
Text Label 4200 1750 0    60   ~ 0
DIG7
Wire Wire Line
	10000 850  10300 850 
Wire Wire Line
	10000 950  10300 950 
Wire Wire Line
	8700 850  8400 850 
Wire Wire Line
	8400 950  8700 950 
Wire Wire Line
	8700 1050 8400 1050
Wire Wire Line
	8400 1150 8700 1150
Wire Wire Line
	8700 1250 8400 1250
Wire Wire Line
	8400 1350 8700 1350
Wire Wire Line
	8400 1450 8700 1450
Wire Wire Line
	8700 1550 8400 1550
Wire Wire Line
	8400 1650 8700 1650
Wire Wire Line
	8700 1750 8400 1750
Wire Wire Line
	8400 1850 8700 1850
Wire Wire Line
	8700 1950 8400 1950
Wire Wire Line
	8400 2050 8700 2050
Wire Wire Line
	8700 2150 8400 2150
Wire Wire Line
	8700 2250 8400 2250
Text Label 8450 850  0    60   ~ 0
A
Text Label 8450 950  0    60   ~ 0
B
Text Label 8450 1050 0    60   ~ 0
C
Text Label 8450 1150 0    60   ~ 0
D
Text Label 8450 1250 0    60   ~ 0
E
Text Label 8450 1350 0    60   ~ 0
F
Text Label 8450 1450 0    60   ~ 0
G1
Text Label 8450 1550 0    60   ~ 0
G2
Text Label 8450 1650 0    60   ~ 0
H
Text Label 8450 1750 0    60   ~ 0
J
Text Label 8450 1850 0    60   ~ 0
K
Text Label 8450 1950 0    60   ~ 0
L
Text Label 8450 2050 0    60   ~ 0
M
Text Label 8450 2150 0    60   ~ 0
N
Text Label 8450 2250 0    60   ~ 0
DP
$Comp
L dc26badge-custom:14_Segment_Dual_CC DS3
U 1 1 5AE677A6
P 9450 3100
F 0 "DS3" H 9750 1900 60  0000 C CNN
F 1 "14_Segment_Dual_CC" H 9400 3600 60  0000 C CNN
F 2 "DC26badge:DUAL_14SEG" H 9450 3100 60  0001 C CNN
F 3 "" H 9450 3100 60  0001 C CNN
	1    9450 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	10000 2750 10300 2750
Wire Wire Line
	10000 2850 10300 2850
Wire Wire Line
	8700 2750 8400 2750
Wire Wire Line
	8400 2850 8700 2850
Wire Wire Line
	8700 2950 8400 2950
Wire Wire Line
	8400 3050 8700 3050
Wire Wire Line
	8700 3150 8400 3150
Wire Wire Line
	8400 3250 8700 3250
Wire Wire Line
	8400 3350 8700 3350
Wire Wire Line
	8700 3450 8400 3450
Wire Wire Line
	8400 3550 8700 3550
Wire Wire Line
	8700 3650 8400 3650
Wire Wire Line
	8400 3750 8700 3750
Wire Wire Line
	8700 3850 8400 3850
Wire Wire Line
	8400 3950 8700 3950
Wire Wire Line
	8700 4050 8400 4050
Wire Wire Line
	8700 4150 8400 4150
Text Label 8450 2750 0    60   ~ 0
A
Text Label 8450 2850 0    60   ~ 0
B
Text Label 8450 2950 0    60   ~ 0
C
Text Label 8450 3050 0    60   ~ 0
D
Text Label 8450 3150 0    60   ~ 0
E
Text Label 8450 3250 0    60   ~ 0
F
Text Label 8450 3350 0    60   ~ 0
G1
Text Label 8450 3450 0    60   ~ 0
G2
Text Label 8450 3550 0    60   ~ 0
H
Text Label 8450 3650 0    60   ~ 0
J
Text Label 8450 3750 0    60   ~ 0
K
Text Label 8450 3850 0    60   ~ 0
L
Text Label 8450 3950 0    60   ~ 0
M
Text Label 8450 4050 0    60   ~ 0
N
Text Label 8450 4150 0    60   ~ 0
DP
$Comp
L dc26badge-custom:14_Segment_Dual_CC DS4
U 1 1 5AE67836
P 9450 4950
F 0 "DS4" H 9750 3750 60  0000 C CNN
F 1 "14_Segment_Dual_CC" H 9400 5450 60  0000 C CNN
F 2 "DC26badge:DUAL_14SEG" H 9450 4950 60  0001 C CNN
F 3 "" H 9450 4950 60  0001 C CNN
	1    9450 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	10000 4600 10300 4600
Wire Wire Line
	10000 4700 10300 4700
Wire Wire Line
	8700 4600 8400 4600
Wire Wire Line
	8400 4700 8700 4700
Wire Wire Line
	8700 4800 8400 4800
Wire Wire Line
	8400 4900 8700 4900
Wire Wire Line
	8700 5000 8400 5000
Wire Wire Line
	8400 5100 8700 5100
Wire Wire Line
	8400 5200 8700 5200
Wire Wire Line
	8700 5300 8400 5300
Wire Wire Line
	8400 5400 8700 5400
Wire Wire Line
	8700 5500 8400 5500
Wire Wire Line
	8400 5600 8700 5600
Wire Wire Line
	8700 5700 8400 5700
Wire Wire Line
	8400 5800 8700 5800
Wire Wire Line
	8700 5900 8400 5900
Wire Wire Line
	8700 6000 8400 6000
Text Label 8450 4600 0    60   ~ 0
A
Text Label 8450 4700 0    60   ~ 0
B
Text Label 8450 4800 0    60   ~ 0
C
Text Label 8450 4900 0    60   ~ 0
D
Text Label 8450 5000 0    60   ~ 0
E
Text Label 8450 5100 0    60   ~ 0
F
Text Label 8450 5200 0    60   ~ 0
G1
Text Label 8450 5300 0    60   ~ 0
G2
Text Label 8450 5400 0    60   ~ 0
H
Text Label 8450 5500 0    60   ~ 0
J
Text Label 8450 5600 0    60   ~ 0
K
Text Label 8450 5700 0    60   ~ 0
L
Text Label 8450 5800 0    60   ~ 0
M
Text Label 8450 5900 0    60   ~ 0
N
Text Label 8450 6000 0    60   ~ 0
DP
$Comp
L dc26badge-custom:14_Segment_Dual_CC DS2
U 1 1 5AE678E9
P 7400 4950
F 0 "DS2" H 7700 3750 60  0000 C CNN
F 1 "14_Segment_Dual_CC" H 7350 5450 60  0000 C CNN
F 2 "DC26badge:DUAL_14SEG" H 7400 4950 60  0001 C CNN
F 3 "" H 7400 4950 60  0001 C CNN
	1    7400 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	7950 4600 8250 4600
Wire Wire Line
	7950 4700 8250 4700
Wire Wire Line
	6650 4600 6350 4600
Wire Wire Line
	6350 4700 6650 4700
Wire Wire Line
	6650 4800 6350 4800
Wire Wire Line
	6350 4900 6650 4900
Wire Wire Line
	6650 5000 6350 5000
Wire Wire Line
	6350 5100 6650 5100
Wire Wire Line
	6350 5200 6650 5200
Wire Wire Line
	6650 5300 6350 5300
Wire Wire Line
	6350 5400 6650 5400
Wire Wire Line
	6650 5500 6350 5500
Wire Wire Line
	6350 5600 6650 5600
Wire Wire Line
	6650 5700 6350 5700
Wire Wire Line
	6350 5800 6650 5800
Wire Wire Line
	6650 5900 6350 5900
Wire Wire Line
	6650 6000 6350 6000
Text Label 6400 4600 0    60   ~ 0
A
Text Label 6400 4700 0    60   ~ 0
B
Text Label 6400 4800 0    60   ~ 0
C
Text Label 6400 4900 0    60   ~ 0
D
Text Label 6400 5000 0    60   ~ 0
E
Text Label 6400 5100 0    60   ~ 0
F
Text Label 6400 5200 0    60   ~ 0
G1
Text Label 6400 5300 0    60   ~ 0
G2
Text Label 6400 5400 0    60   ~ 0
H
Text Label 6400 5500 0    60   ~ 0
J
Text Label 6400 5600 0    60   ~ 0
K
Text Label 6400 5700 0    60   ~ 0
L
Text Label 6400 5800 0    60   ~ 0
M
Text Label 6400 5900 0    60   ~ 0
N
Text Label 6400 6000 0    60   ~ 0
DP
Text Label 10250 850  2    60   ~ 0
DIG0
Text Label 10250 950  2    60   ~ 0
DIG1
Text Label 10250 2750 2    60   ~ 0
DIG4
Text Label 10250 2850 2    60   ~ 0
DIG5
Text Label 10250 4600 2    60   ~ 0
DIG6
Text Label 10250 4700 2    60   ~ 0
DIG7
Text Label 8200 4600 2    60   ~ 0
DIG2
Text Label 8200 4700 2    60   ~ 0
DIG3
$Comp
L DC26Badge-rescue:C_Small C1
U 1 1 5AE67BDA
P 6300 850
F 0 "C1" H 6310 920 50  0000 L CNN
F 1 "0.1u" H 6310 770 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 6300 850 50  0001 C CNN
F 3 "" H 6300 850 50  0001 C CNN
	1    6300 850 
	-1   0    0    1   
$EndComp
$Comp
L power:VDD #PWR03
U 1 1 5AE67C8F
P 6300 700
F 0 "#PWR03" H 6300 550 50  0001 C CNN
F 1 "VDD" H 6300 850 50  0000 C CNN
F 2 "" H 6300 700 50  0001 C CNN
F 3 "" H 6300 700 50  0001 C CNN
	1    6300 700 
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR04
U 1 1 5AE67CAF
P 6300 1000
F 0 "#PWR04" H 6300 750 50  0001 C CNN
F 1 "GND" H 6300 850 50  0000 C CNN
F 2 "" H 6300 1000 50  0001 C CNN
F 3 "" H 6300 1000 50  0001 C CNN
	1    6300 1000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6300 700  6300 750 
Wire Wire Line
	6300 950  6300 1000
$EndSCHEMATC
