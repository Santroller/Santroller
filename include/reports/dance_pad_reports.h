#include <stdint.h>
#define STEPMANIA_X_REPORT_ID 3
typedef struct {
    uint8_t reportId;  // 0x01
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadCenter : 1;
    uint8_t : 3;

    uint8_t : 2;
    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t : 4;
} __attribute__((packed)) LTEK_Report_With_Id_Data_t;
typedef struct {
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadCenter : 1;
    uint8_t : 3;

    uint8_t : 2;
    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t : 4;
} __attribute__((packed)) LTEK_Report_Data_t;
typedef struct {
    uint8_t reportId;  // 0x03
    uint8_t dpadUpLeft : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadUpRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadCenter : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadDownLeft : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadDownRight : 1;
    uint8_t padding : 7;
} __attribute__((packed)) StepManiaX_Report_Data_t;