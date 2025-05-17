#ifndef Controller_h
#define Controller_h
#include <stdio.h>

#include "hardware/pio.h"
#include "pico/stdlib.h"

typedef enum { N64 = 0x05, Gamecube = 0x09 } ControllerType;

typedef struct {
  uint8_t pin;
  ControllerType controllerType;
  PIO pio;
  uint sm;
  pio_sm_config *c;
  uint offset;
} InitParams;

class Controller {
 public:
  Controller(InitParams *initParams, uint8_t sizeofControllerState);
  static void initPio(InitParams *initParams);
  void transfer(uint8_t *request, uint8_t requestLength, uint8_t *response,
                uint8_t responseLength);
  double getScaledAnalogAxis(double axisPos, double *minAxis, double *maxAxis);
  virtual void init() = 0;
  virtual void setRumble(bool rumble) = 0;

 private:
  static void transfer(PIO pio, uint sm, uint8_t *request,
                       uint8_t requestLength, uint8_t *response,
                       uint8_t responseLength);
  static void sendRequest(PIO pio, uint sm, uint8_t *request,
                          uint8_t requestLength);
  static void getResponse(PIO pio, uint sm, uint8_t *response,
                          uint8_t responseLength);

 protected:
  virtual void updateState() = 0;
  uint8_t _pin;
  PIO _pio;
  uint _sm;
  pio_sm_config *_c;
  uint _offset;
  uint8_t *_controllerState;
  uint8_t _sizeofControllerState;
  bool _rumble = false;
};

#endif