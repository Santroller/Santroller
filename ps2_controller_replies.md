PSX Controller Data
(from https://pinouts.ru/Game/playstation_9_pinout.shtml)

Below are five tables that show the actual bytes sent by the controllers

    Standard Digital Pad

    BYTE    CMND    DATA

     01     0x01    idle

     02     0x42    0x41

     03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7

     04     idle    data    SLCT  JOYL  JOYR       STRT UP   RGHT DOWN LEFT

     05     idle    data    L1   R1    L2  R2   /   O    X    |_|

    All Buttons active low.

    NegCon

    BYTE    CMND    DATA

     01     0x01    idle

     02     0x42    0x23

     03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7

     04     idle    data                   STRT UP   RGHT DOWN LEFT

     05     idle    data                   R1   A    B

     06     idle    data    Steering  0x00 = Right 0xFF = Left

     07     idle    data    I  Button 0x00 = Out   0xFF = In

     08     idle    data    II Button 0x00 = Out   0xFF = In

     09     idle    data    L1 Button 0x00 = Out   0xFF = In

    All Buttons active low.

    Analogue Controller in Red Mode

    BYTE    CMND    DATA

     01     0x01    idle

     02     0x42    0x73

     03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7

     04     idle    data    SLCT  JOYL  JOYR       STRT UP   RGHT DOWN LEFT

     05     idle    data    L1   R1    L2  R2   /   O    X    |_|

     06     idle    data    Right Joy 0x00 = Left  0xFF = Right

     07     idle    data    Right Joy 0x00 = Up    0xFF = Down

     08     idle    data    Left Joy  0x00 = Left  0xFF = Right

     09     idle    data    Left Joy  0x00 = Up    0xFF = Down

    All Buttons active low.


    Analogue Controller in Green Mode

    BYTE    CMND    DATA

     01     0x01    idle

     02     0x42    0x53

     03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7

     04     idle    data                   STRT UP   RGHT DOWN LEFT

     05     idle    data    L2   L1   |_|  /   R1   O    X    R2

     06     idle    data    Right Joy 0x00 = Left  0xFF = Right

     07     idle    data    Right Joy 0x00 = Up    0xFF = Down

     08     idle    data    Left Joy  0x00 = Left  0xFF = Right

     09     idle    data    Left Joy  0x00 = Up    0xFF = Down

    All Buttons active low.

    PSX Mouse 

    BYTE    CMND    DATA

     01     0x01    idle

     02     0x42    0x12

     03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7

     04     idle    0xFF

     05     idle    data         L    R

     06     idle    data    Delta Vertical

     07     idle    data    Delta Horizontal

    All Buttons active low.

 