#ifdef INPUT_DJ_TURNTABLE
uint8_t *dj_left = lastSuccessfulTurntablePacketLeft;
uint8_t *dj_right = lastSuccessfulTurntablePacketRight;
bool djLeftValid = twi_readFromPointer(DJ_TWI_PORT, DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketLeft), dj_left);
bool djRightValid = twi_readFromPointer(DJ_TWI_PORT, DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketRight), dj_right);
lastTurntableWasSuccessfulLeft = djLeftValid;
lastTurntableWasSuccessfulRight = djRightValid;
#endif