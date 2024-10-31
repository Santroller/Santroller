#include <stdbool.h>
#include <stdint.h>

#include "config.h"
typedef struct {
    union {
        struct {
            uint8_t rightStickX0 : 1;
            uint8_t rightStickX21 : 2;
            uint8_t rightStickX43 : 2;
        };
        uint8_t rightStickX : 5;
    };
    union {
        struct {
            uint8_t leftTrigger20 : 3;
            uint8_t leftTrigger43 : 2;
        };
        uint8_t leftTrigger : 5;
    };
} __attribute__((packed)) WiiIntermediateClassicDataFormat_t;
typedef struct {
    uint8_t leftStickX : 6;
    uint8_t rightStickX43 : 2;
    uint8_t leftStickY : 6;
    uint8_t rightStickX21 : 2;
    uint8_t rightStickY : 5;
    uint8_t leftTrigger43 : 2;
    uint8_t rightStickX0 : 1;
    uint8_t rightTrigger : 5;
    uint8_t leftTrigger20 : 3;
    union {
        struct {
            uint8_t : 1;
            uint8_t r2 : 1;
            uint8_t start : 1;
            uint8_t guide : 1;
            uint8_t back : 1;
            uint8_t l2 : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadRight : 1;
        };
        uint8_t buttonsLow;
    };
    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadLeft : 1;
            uint8_t rightShoulder : 1;
            uint8_t x : 1;
            uint8_t a : 1;
            uint8_t y : 1;
            uint8_t b : 1;
            uint8_t leftShoulder : 1;
        };
        uint8_t buttonsHigh;
    };
} __attribute__((packed)) WiiClassicDataFormat1_t;
typedef struct {
    uint8_t leftStickX92;
    uint8_t rightStickX92;
    uint8_t leftStickY92;
    uint8_t rightStickY92;
    uint8_t leftStickX10 : 2;
    uint8_t rightStickX10 : 2;
    uint8_t leftStickY10 : 2;
    uint8_t rightStickY10 : 2;
    uint8_t leftTrigger;
    uint8_t rightTrigger;

    union {
        struct {
            uint8_t : 1;
            uint8_t r2 : 1;
            uint8_t start : 1;
            uint8_t guide : 1;
            uint8_t back : 1;
            uint8_t l2 : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadRight : 1;
        };
        uint8_t buttonsLow;
    };
    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadLeft : 1;
            uint8_t rightShoulder : 1;
            uint8_t x : 1;
            uint8_t a : 1;
            uint8_t y : 1;
            uint8_t b : 1;
            uint8_t leftShoulder : 1;
        };
        uint8_t buttonsHigh;
    };
} __attribute__((packed)) WiiClassicDataFormat2_t;
typedef struct {
    uint8_t leftStickX;
    uint8_t rightStickX;
    uint8_t leftStickY;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;

    union {
        struct {
            uint8_t : 1;
            uint8_t r2 : 1;
            uint8_t start : 1;
            uint8_t guide : 1;
            uint8_t back : 1;
            uint8_t l2 : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadRight : 1;
        };
        uint8_t buttonsLow;
    };
    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadLeft : 1;
            uint8_t rightShoulder : 1;
            uint8_t x : 1;
            uint8_t a : 1;
            uint8_t y : 1;
            uint8_t b : 1;
            uint8_t leftShoulder : 1;
        };
        uint8_t buttonsHigh;
    };
} __attribute__((packed)) WiiClassicDataFormat3_t;

typedef struct {
    uint8_t leftStickX : 6;
    uint8_t : 2;

    uint8_t leftStickY : 6;
    uint8_t : 2;

    uint8_t slider : 5;
    uint8_t : 3;

    uint8_t whammy : 5;
    uint8_t : 3;

    uint8_t : 1;
    uint8_t : 1;
    uint8_t start : 1;
    uint8_t guide : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadLeft : 1;
    uint8_t kick1 : 1;
    uint8_t yellow : 1;
    uint8_t green : 1;
    uint8_t blue : 1;
    uint8_t red : 1;
    uint8_t orange : 1;
} __attribute__((packed)) WiiGuitarDataFormat3_t;

typedef struct {
    uint8_t leftStickX : 6;
    uint8_t : 2;
    uint8_t leftStickY : 6;
    uint8_t : 2;
    uint8_t : 1;
    uint8_t which : 5;
    uint8_t : 1;
    uint8_t hhp;
    uint8_t : 5;
    uint8_t velocity : 3;
    uint8_t : 8;

    uint8_t : 1;
    uint8_t : 1;
    uint8_t start : 1;
    uint8_t guide : 1;
    uint8_t back : 1;
    uint8_t : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadLeft : 1;
    uint8_t kick1 : 1;
    uint8_t blue : 1;
    uint8_t green : 1;
    uint8_t yellow : 1;
    uint8_t red : 1;
    uint8_t orange : 1;
} __attribute__((packed)) WiiDrumDataFormat3_t;

typedef struct {
    uint8_t leftStickX : 6;
    uint8_t : 2;
    uint8_t leftStickY : 6;
    uint8_t : 2;
    union {
        struct {
            int8_t leftTableVelocity : 6;
            int8_t : 2;
        };
        struct {
            uint8_t leftTableVelocity40 : 5;
            uint8_t leftTableVelocity5 : 1;
            uint8_t : 2;
        };
    };
    union {
        struct {
            int8_t rightTableVelocity : 6;
            int8_t : 2;
        };
        struct {
            uint8_t rightTableVelocity0 : 1;
            uint8_t rightTableVelocity21 : 2;
            uint8_t rightTableVelocity43 : 2;
            uint8_t rightTableVelocity5 : 1;
            uint8_t : 2;
        };
    };
    uint8_t crossfader : 4;
    uint8_t : 4;

    union {
        struct {
            uint8_t effectsKnob20 : 3;
            uint8_t effectsKnob43 : 2;
        };
        uint8_t effectsKnob : 5;
    };
    uint8_t : 3;

    union {
        struct {
            uint8_t : 1;
            uint8_t rightRed : 1;
            uint8_t start : 1;
            uint8_t guide : 1;
            uint8_t back : 1;
            uint8_t leftRed : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadRight : 1;
        };
        uint8_t buttonsLow;
    };
    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadLeft : 1;
            uint8_t rightBlue : 1;
            uint8_t leftGreen : 1;
            uint8_t euphoria : 1;
            uint8_t rightGreen : 1;
            uint8_t : 1;
            uint8_t leftBlue : 1;
        };
        uint8_t buttonsHigh;
    };
} __attribute__((packed)) WiiTurntableIntermediateFormat3_t;

typedef struct {
    uint8_t leftStickX : 6;
    uint8_t rightTableVelocity43 : 2;
    uint8_t leftStickY : 6;
    uint8_t rightTableVelocity21 : 2;
    uint8_t rightTableVelocity5 : 1;
    uint8_t crossfader : 4;
    uint8_t effectsKnob43 : 2;
    uint8_t rightTableVelocity0 : 1;
    uint8_t leftTableVelocity40 : 5;
    uint8_t effectsKnob20 : 3;

    union {
        struct {
            uint8_t leftTableVelocity5 : 1;
            uint8_t rightRed : 1;
            uint8_t start : 1;
            uint8_t guide : 1;
            uint8_t back : 1;
            uint8_t leftRed : 1;
            uint8_t dpadDown : 1;
            uint8_t dpadRight : 1;
        };
        uint8_t buttonsLow;
    };
    union {
        struct {
            uint8_t dpadUp : 1;
            uint8_t dpadLeft : 1;
            uint8_t rightBlue : 1;
            uint8_t leftGreen : 1;
            uint8_t y : 1;  // euphoria
            uint8_t rightGreen : 1;
            uint8_t : 1;
            uint8_t leftBlue : 1;
        };
        uint8_t buttonsHigh;
    };
} __attribute__((packed)) WiiTurntableDataFormat3_t;

typedef struct {
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t accelX92;
    uint8_t accelY92;
    uint8_t accelZ92;
    uint8_t z : 1;
    uint8_t c : 1;
    uint8_t accelX10 : 2;
    uint8_t accelY10 : 2;
    uint8_t accelZ10 : 2;
} __attribute__((packed)) WiiNunchukDataFormat_t;

typedef struct {
    uint8_t x70;
    uint8_t y70;
    uint8_t x118;
    uint8_t y118;
    uint8_t pressure70;
    uint8_t : 8;
    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t pressure8 : 1;
    uint8_t : 5;
} __attribute__((packed)) WiiUDrawDataFormat_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
    uint8_t : 8;
    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t click : 1;
    uint8_t : 5;
} __attribute__((packed)) WiiDrawsomeDataFormat_t;

typedef struct {
    uint8_t : 3;
    uint8_t rimRight;
    uint8_t centerRight;
    uint8_t rimLeft;
    uint8_t centerLeft;
    uint8_t : 1;
} __attribute__((packed)) WiiTaTaConDataFormat_t;