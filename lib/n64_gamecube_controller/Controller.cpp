#include "Controller.h"

#include "Controller.pio.h"

Controller::Controller(InitParams *initParams, uint8_t sizeofControllerState) {
  _pin = initParams->pin;
  _pio = initParams->pio;
  _sm = initParams->sm;
  _c = initParams->c;
  _offset = initParams->offset;
  _sizeofControllerState = sizeofControllerState;
  _controllerState = new uint8_t[_sizeofControllerState];
}

void Controller::initPio(InitParams *initParams) {
  pio_hw_t *pios[2] = {pio0, pio1};
  uint pio_index = 0;
  if (!pio_can_add_program(pios[pio_index], &controller_program)) {
    pio_index = 1;
    if (!pio_can_add_program(pios[pio_index], &controller_program)) {
      // throw 0;
    }
  }
  initParams->pio = pios[pio_index];
  initParams->offset = pio_add_program(initParams->pio, &controller_program);
  initParams->sm = pio_claim_unused_sm(initParams->pio, true);
  pio_sm_config tmpConfig =
      controller_program_get_default_config(initParams->offset);
  initParams->c = &tmpConfig;
  controller_program_init(initParams->pio, initParams->sm, initParams->offset,
                          initParams->pin, initParams->c);

  // Send a command to see if connected controller is N64 or Gamecube
  uint8_t initRequest[1] = {0x00};
  uint8_t initResponse[3] = {0x00};
  transfer(initParams->pio, initParams->sm, initRequest, sizeof(initRequest),
           initResponse, sizeof(initResponse));
  initParams->controllerType = (ControllerType)initResponse[0];
}
void Controller::transfer(uint8_t *request, uint8_t requestLength,
                          uint8_t *response, uint8_t responseLength) {
  transfer(_pio, _sm, request, requestLength, response, responseLength);
}

void Controller::transfer(PIO pio, uint sm, uint8_t *request,
                          uint8_t requestLength, uint8_t *response,
                          uint8_t responseLength) {
  pio_sm_clear_fifos(pio, sm);
  pio_sm_put_blocking(pio, sm, ((responseLength - 1) & 0x1F) << 24);
  sendRequest(pio, sm, request, requestLength);
  getResponse(pio, sm, response, responseLength);
  busy_wait_us(4 * (requestLength + responseLength) + 450);
}

void Controller::sendRequest(PIO pio, uint sm, uint8_t *request,
                             uint8_t requestLength) {
  int8_t remainingRequestBytes = requestLength;
  while (remainingRequestBytes > 0) {
    pio_sm_put_blocking(pio, sm,
                        request[requestLength - remainingRequestBytes] << 24);
    remainingRequestBytes--;
  }
}

void Controller::getResponse(PIO pio, uint sm, uint8_t *response,
                             uint8_t responseLength) {
  int16_t remainingResponseBytes = responseLength;
  while (remainingResponseBytes > 0) {
    absolute_time_t timeout_us = make_timeout_time_us(600);
    bool timedOut = false;
    while (pio_sm_is_rx_fifo_empty(pio, sm) && !timedOut) {
      timedOut = time_reached(timeout_us);
    }
    if (timedOut) {
      // throw 0;
    }
    uint32_t data = pio_sm_get(pio, sm);
    response[responseLength - remainingResponseBytes] = (uint8_t)(data & 0xFF);
    remainingResponseBytes--;
  }
}

double Controller::getScaledAnalogAxis(double axisPos, double *minAxis,
                                       double *maxAxis) {
  *maxAxis = axisPos > *maxAxis ? axisPos : *maxAxis;
  *minAxis = axisPos < *minAxis ? axisPos : *minAxis;
  return axisPos > 0 ? axisPos / *maxAxis : -axisPos / *minAxis;
}