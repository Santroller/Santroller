#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t back;
  uint8_t left_stick;
  uint8_t right_stick;
  uint8_t LB;
  uint8_t RB;
  uint8_t home;
  uint8_t capture;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t lt;
  uint8_t rt;
  uint8_t l_x;
  uint8_t l_y;
  uint8_t r_x;
  uint8_t r_y;
} pins_t;
typedef struct {
  bool lt;
  bool rt;
  bool l_x;
  bool l_y;
  bool r_x;
  bool r_y;
} axis_invert_t;
typedef struct {
  uint8_t neg;
  uint8_t pos;
} analogue_keys_t;
typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t back;
  uint8_t left_stick;
  uint8_t right_stick;
  uint8_t LB;
  uint8_t RB;
  uint8_t home;
  uint8_t capture;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t lt;
  uint8_t rt;
  analogue_keys_t l_x;
  analogue_keys_t l_y;
  analogue_keys_t r_x;
  analogue_keys_t r_y;
} keys_t;
typedef struct {
  uint8_t input_type;
  uint8_t sub_type;
  uint8_t tilt_type;
  uint8_t poll_rate;
  uint8_t frets_led_mode;
  bool map_joy_to_dpad;
  bool map_start_select_to_home;
  bool map_accel_to_right;
} main_config_t;
typedef struct {
  axis_invert_t inversions;
  uint16_t threshold_trigger;
  uint16_t threshold_joy;
  uint8_t mpu_6050_orientation;
  int16_t tilt_sensitivity;
} axis_config_t;
typedef struct {
  main_config_t main;
  pins_t pins;
  axis_config_t axis;
  keys_t keys;
} config_t;

#define MAIN_CMD_R 'm'
#define MAIN_CMD_W 'n'
#define PIN_CMD_R 'p'
#define PIN_CMD_W 'o'
#define AXIS_CMD_R 'a'
#define AXIS_CMD_W 'b'
#define KEY_CMD_R 'k'
#define KEY_CMD_W 'l'
#define FW_CMD_R 'f'
#define CONTROLLER_CMD_R 'c'
#define REBOOT_CMD 'r'
#define BOOTLOADER_CMD 'd'