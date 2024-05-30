#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WII_ADDR 0x52
#define WII_READ_ID 0xFA
#define WII_ENCRYPTION_ID 0xF0
#define WII_ENCRYPTION_KEY_ID 0x40
#define WII_ENCRYPTION_KEY_ID_2 0x46
#define WII_ENCRYPTION_KEY_ID_3 0x4C
#define WII_ID_LEN 6
#define WII_DJ_EUPHORIA 0xFB
#define WII_SET_RES_MODE 0xFE
#define WII_LOWRES_MODE 0x03
#define WII_HIGHRES_MODE 0x03
enum DrumType {
    DRUM_GREEN,
    DRUM_RED,
    DRUM_YELLOW,
    DRUM_BLUE,
    DRUM_ORANGE,
    DRUM_KICK,
    DRUM_HIHAT
};
extern bool hiRes;
extern uint16_t wiiControllerType;
uint8_t* tickWii();
bool wiiDataValid();
#ifdef __cplusplus
}
#endif