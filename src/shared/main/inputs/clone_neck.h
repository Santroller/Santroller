#ifdef INPUT_CLONE_NECK
uint8_t *clone_buttons = lastSuccessfulClonePacket;
bool cloneValid = twi_writeTo(CLONE_TWI_PORT, CLONE_ADDR, clone_data, sizeof(clone_data), true, true) && twi_readFrom(CLONE_TWI_PORT, CLONE_ADDR, lastSuccessfulClonePacket, sizeof(lastSuccessfulClonePacket), lastSuccessfulClonePacket);
lastCloneWasSuccessful = cloneValid;
#endif