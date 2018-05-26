EESchema Schematic File Version 4
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
L Connector_Generic:Conn_02x05_Odd_Even J1
U 1 1 5B076888
P 3200 2150
F 0 "J1" H 3250 2567 50  0000 C CNN
F 1 "Conn_02x05_Odd_Even" H 3250 2476 50  0000 C CNN
F 2 "Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical_SMD" H 3200 2150 50  0001 C CNN
F 3 "~" H 3200 2150 50  0001 C CNN
	1    3200 2150
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J2
U 1 1 5B0769F6
P 4200 2050
F 0 "J2" H 4280 2042 50  0000 L CNN
F 1 "Conn_01x04" H 4280 1951 50  0000 L CNN
F 2 "DC26badge:SWD_POGO" H 4200 2050 50  0001 C CNN
F 3 "~" H 4200 2050 50  0001 C CNN
	1    4200 2050
	1    0    0    -1  
$EndComp
Wire Wire Line
	4000 1950 4000 1550
Wire Wire Line
	4000 1550 2750 1550
Wire Wire Line
	2750 1550 2750 1950
Wire Wire Line
	2750 1950 3000 1950
Wire Wire Line
	3000 2050 2750 2050
Wire Wire Line
	2750 2050 2750 2150
Wire Wire Line
	2750 2350 3000 2350
Wire Wire Line
	3000 2150 2750 2150
Connection ~ 2750 2150
Wire Wire Line
	2750 2150 2750 2350
Wire Wire Line
	2750 2350 2750 2600
Wire Wire Line
	2750 2600 4000 2600
Wire Wire Line
	4000 2600 4000 2250
Connection ~ 2750 2350
Wire Wire Line
	4000 2050 3850 2050
Wire Wire Line
	3850 2050 3850 1950
Wire Wire Line
	3850 1950 3500 1950
Wire Wire Line
	4000 2150 3700 2150
Wire Wire Line
	3700 2150 3700 2050
Wire Wire Line
	3700 2050 3500 2050
NoConn ~ 3000 2250
NoConn ~ 3500 2150
NoConn ~ 3500 2250
NoConn ~ 3500 2350
$EndSCHEMATC
