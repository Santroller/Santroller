#pragma once
#include "controller/controller.h"
#include "util/util.h"
#define GH5NECK_ADDR 0x0D
#define GH5NECK_OK_PTR 0x11
#define GH5NECK_BUTTONS_PTR 0x12
// Extended GH5 slider with full multi-touch
#define GH5NECK_SLIDER_NEW_PTR 0x15
// Older style slider with WT-type detection, adjacent frets only
#define GH5NECK_SLIDER_OLD_PTR 0x16
void initGuitar(void);
void tickGuitar(Controller_t* controller);
bool isGuitar(void);
bool isDrum(void);
