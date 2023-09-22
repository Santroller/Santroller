#ifdef INPUT_CLONE_NECK
uint8_t clone_data_read[4];
uint8_t *clone_buttons = lastSuccessfulClonePacket;
if (!clone_guitar_ready_timer) {
    clone_guitar_ready_timer = millis();
}
if (millis() - clone_guitar_ready_timer > 350) {
    clone_ready = true;
}
if (clone_ready) {
    // The crazy guitar necks don't like being polled too quickly, this seems to work though.
    if (micros() - clone_guitar_timer > 4000) {
        if (!reading) {
            twi_writeTo(CLONE_TWI_PORT, CLONE_ADDR, clone_data, sizeof(clone_data), true, true);
            reading = true;
        } else {
            bool cloneValid = twi_readFrom(CLONE_TWI_PORT, CLONE_ADDR, clone_data_read, sizeof(clone_data_read), true);
            lastCloneWasSuccessful = cloneValid;
            if (!cloneValid) {
                clone_ready = false;
                clone_guitar_ready_timer = millis();
            }
            if (clone_data_read[0] == CLONE_VALID_PACKET) {
                memcpy(lastSuccessfulClonePacket, clone_data_read, sizeof(clone_data_read));
                reading = false;
            }
        }
        clone_guitar_timer = micros();
    }
}

#endif