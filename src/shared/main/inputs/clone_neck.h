#ifdef INPUT_CLONE_NECK
uint8_t clone_data_read[4];
uint8_t *clone_buttons = lastSuccessfulClonePacket;
bool cloneValid = twi_writeTo(CLONE_TWI_PORT, CLONE_ADDR, clone_data, sizeof(clone_data), true, true) && twi_readFrom(CLONE_TWI_PORT, CLONE_ADDR, clone_data_read, sizeof(clone_data_read), true);
lastCloneWasSuccessful = cloneValid;
if (clone_data_read[0] == CLONE_VALID_PACKET) {
    memcpy(lastSuccessfulClonePacket, clone_data_read, sizeof(clone_data_read));
}
#endif