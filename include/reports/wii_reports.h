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
    uint8_t leftStickX : 5;
    uint8_t : 2;
    uint8_t leftStickY : 5;
    uint8_t : 2;
    uint8_t slider : 4;
    uint8_t : 3;
    uint8_t whammy : 4;
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
    uint8_t rightShoulder : 1;
    uint8_t x : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t leftShoulder : 1;
} __attribute__((packed)) WiiGuitarDataFormat3_t;

typedef struct {
    uint8_t leftStickX : 5;
    uint8_t : 2;
    uint8_t leftStickY : 5;
    uint8_t : 2;
    // Velocity is here but i don't want to emulate that right now
    uint8_t : 8;
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
    uint8_t rightShoulder : 1;
    uint8_t x : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t leftShoulder : 1;
} __attribute__((packed)) WiiDrumDataFormat3_t;

typedef struct {
    uint8_t leftStickX : 6;
    uint8_t : 2;
    uint8_t leftStickY : 6;
    uint8_t : 2;
    union {
        signed int leftTableVelocity : 6;
        struct {
            unsigned int leftTableVelocity40 : 5;
            unsigned int leftTableVelocity5 : 1;
        };
    };
    uint8_t : 2;
    union {
        signed int rightTableVelocity : 6;
        struct {
            unsigned int rightTableVelocity0 : 1;
            unsigned int rightTableVelocity21 : 2;
            unsigned int rightTableVelocity43 : 2;
            unsigned int rightTableVelocity5 : 1;
        };
    };
    uint8_t : 2;
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
            uint8_t y : 1;  // euphoria
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