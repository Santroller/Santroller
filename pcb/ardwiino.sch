EESchema Schematic File Version 4
LIBS:ardwiino-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Ardwiino"
Date "2019-01-25"
Rev "1.0"
Comp "Sebastiaan Jansen & Sanjay Govind"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L nunchuck:NUNCHUCKLOCL M1
U 1 1 5BB605F7
P 8950 2450
F 0 "M1" H 8950 2450 45  0001 C CNN
F 1 "Top" H 8950 2450 45  0001 C CNN
F 2 "nunchuck:HHJ-NUNCHUCK-LOCK" H 8980 2600 20  0001 C CNN
F 3 "" H 8950 2450 50  0001 C CNN
	1    8950 2450
	1    0    0    -1  
$EndComp
$Comp
L promicro:ProMicro 3.3V1
U 1 1 5BB743DF
P 4250 3650
F 0 "3.3V1" H 4250 4687 60  0000 C CNN
F 1 "ProMicro" H 4250 4581 60  0000 C CNN
F 2 "ProMicro:ProMicro" H 4350 2600 60  0001 C CNN
F 3 "" H 4350 2600 60  0000 C CNN
	1    4250 3650
	1    0    0    -1  
$EndComp
$Comp
L gy-521:GY-521 MPU6050
U 1 1 5C4AF883
P 7300 3400
F 0 "MPU6050" H 6800 3750 60  0000 C CNN
F 1 "GY-521" H 6800 3650 60  0000 C CNN
F 2 "gy521:GY521" H 7300 3400 60  0001 C CNN
F 3 "" H 7300 3400 60  0000 C CNN
	1    7300 3400
	1    0    0    -1  
$EndComp
Text Notes 8800 2050 0    50   ~ 0
Nunchuck
$Comp
L power:GND #PWR0101
U 1 1 5C4B1B5A
P 9650 2650
F 0 "#PWR0101" H 9650 2400 50  0001 C CNN
F 1 "GND" H 9655 2477 50  0000 C CNN
F 2 "" H 9650 2650 50  0001 C CNN
F 3 "" H 9650 2650 50  0001 C CNN
	1    9650 2650
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5C4B1B89
P 3350 1250
F 0 "#PWR0102" H 3350 1000 50  0001 C CNN
F 1 "GND" H 3355 1077 50  0000 C CNN
F 2 "" H 3350 1250 50  0001 C CNN
F 3 "" H 3350 1250 50  0001 C CNN
	1    3350 1250
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0103
U 1 1 5C4B1BFE
P 2850 1050
F 0 "#PWR0103" H 2850 900 50  0001 C CNN
F 1 "+3.3V" H 2865 1223 50  0000 C CNN
F 2 "" H 2850 1050 50  0001 C CNN
F 3 "" H 2850 1050 50  0001 C CNN
	1    2850 1050
	1    0    0    -1  
$EndComp
Text Label 3550 3300 2    50   ~ 0
SCL
Text Label 3550 3400 2    50   ~ 0
SDA
$Comp
L power:+3.3V #PWR0104
U 1 1 5C4C4919
P 8350 2250
F 0 "#PWR0104" H 8350 2100 50  0001 C CNN
F 1 "+3.3V" H 8365 2423 50  0000 C CNN
F 2 "" H 8350 2250 50  0001 C CNN
F 3 "" H 8350 2250 50  0001 C CNN
	1    8350 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	9550 2550 9650 2550
Wire Wire Line
	9650 2550 9650 2650
Wire Wire Line
	8450 2350 8350 2350
Wire Wire Line
	8350 2350 8350 2250
Text Label 9550 2350 0    50   ~ 0
SCL
Text Label 8450 2550 2    50   ~ 0
SDA
Text Label 7850 3300 0    50   ~ 0
SCL
Text Label 7850 3400 0    50   ~ 0
SDA
$Comp
L power:+3.3V #PWR0106
U 1 1 5C4C5913
P 7300 2800
F 0 "#PWR0106" H 7300 2650 50  0001 C CNN
F 1 "+3.3V" H 7315 2973 50  0000 C CNN
F 2 "" H 7300 2800 50  0001 C CNN
F 3 "" H 7300 2800 50  0001 C CNN
	1    7300 2800
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x12 ProMicro_Left1
U 1 1 5C4C5B26
P 2650 3400
F 0 "ProMicro_Left1" H 2867 3346 50  0000 L BNN
F 1 "Conn_01x12" H 2776 3346 50  0000 L TNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x12_P2.54mm_Vertical" H 2650 3400 50  0001 C CNN
F 3 "~" H 2650 3400 50  0001 C CNN
	1    2650 3400
	-1   0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x12 ProMicro_Right1
U 1 1 5C4C5D60
P 5650 3400
F 0 "ProMicro_Right1" H 5730 3392 50  0000 L CNN
F 1 "Conn_01x12" H 5730 3301 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x12_P2.54mm_Vertical" H 5650 3400 50  0001 C CNN
F 3 "~" H 5650 3400 50  0001 C CNN
	1    5650 3400
	1    0    0    -1  
$EndComp
Text Label 3550 2900 2    50   ~ 0
TX
Text Label 3550 3000 2    50   ~ 0
RX
Text Label 3350 1150 2    50   ~ 0
GND
Wire Wire Line
	3350 1250 3350 1150
Text Label 3550 3100 2    50   ~ 0
GND
Text Label 3550 3200 2    50   ~ 0
GND
Text Label 3550 3500 2    50   ~ 0
D4
Text Label 3550 3600 2    50   ~ 0
C6
Text Label 3550 3700 2    50   ~ 0
D7
Text Label 3550 3800 2    50   ~ 0
E6
Text Label 3550 3900 2    50   ~ 0
B4
Text Label 3550 4000 2    50   ~ 0
B5
Text Label 4950 2900 0    50   ~ 0
RAW
Text Label 4950 3000 0    50   ~ 0
GND
Text Label 4950 3100 0    50   ~ 0
RST
Text Label 4950 3200 0    50   ~ 0
VCC
Text Label 4950 3300 0    50   ~ 0
F4
Text Label 4950 3400 0    50   ~ 0
F5
Text Label 4950 3500 0    50   ~ 0
F6
Text Label 4950 3600 0    50   ~ 0
F7
Text Label 4950 3700 0    50   ~ 0
B1
Text Label 4950 3800 0    50   ~ 0
B3
Text Label 4950 3900 0    50   ~ 0
B2
Text Label 4950 4000 0    50   ~ 0
B6
Text Label 2850 2900 0    50   ~ 0
TX
Text Label 2850 3000 0    50   ~ 0
RX
Text Label 2850 3100 0    50   ~ 0
GND
Text Label 2850 3200 0    50   ~ 0
GND
Text Label 2850 3300 0    50   ~ 0
SCL
Text Label 2850 3400 0    50   ~ 0
SDA
Text Label 2850 3500 0    50   ~ 0
D4
Text Label 2850 3600 0    50   ~ 0
C6
Text Label 2850 3700 0    50   ~ 0
D7
Text Label 2850 3800 0    50   ~ 0
E6
Text Label 2850 3900 0    50   ~ 0
B4
Text Label 2850 4000 0    50   ~ 0
B5
Text Label 5450 2900 2    50   ~ 0
RAW
Text Label 5450 3000 2    50   ~ 0
GND
Text Label 5450 3100 2    50   ~ 0
RST
Text Label 5450 3200 2    50   ~ 0
VCC
Text Label 5450 3300 2    50   ~ 0
F4
Text Label 5450 3400 2    50   ~ 0
F5
Text Label 5450 3500 2    50   ~ 0
F6
Text Label 5450 3600 2    50   ~ 0
F7
Text Label 5450 3700 2    50   ~ 0
B1
Text Label 5450 3800 2    50   ~ 0
B3
Text Label 5450 3900 2    50   ~ 0
B2
Text Label 5450 4000 2    50   ~ 0
B6
Text Label 2850 1150 2    50   ~ 0
VCC
Wire Wire Line
	2850 1150 2850 1050
$Comp
L power:GND #PWR0105
U 1 1 5C4C57EB
P 7300 4000
F 0 "#PWR0105" H 7300 3750 50  0001 C CNN
F 1 "GND" H 7305 3827 50  0000 C CNN
F 2 "" H 7300 4000 50  0001 C CNN
F 3 "" H 7300 4000 50  0001 C CNN
	1    7300 4000
	1    0    0    -1  
$EndComp
$EndSCHEMATC
