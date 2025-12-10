#include "band_hero_drum.hpp"
#include "main.hpp"
#include "utils.h"
void BandHeroDrum::tick()
{
    uint8_t start[2] = {0};
    uint8_t data[MAX_PACKETS_IN_BUFFER * PACKET_SIZE] = {0};
    connected = interface.readRegisterRepeatedStart(DRUM_ADDR, BH_DRUM_PTR, sizeof(start), start);
    if (connected)
    {
        uint8_t numPackets = start[1] >> 4;
        if (numPackets)
        {
            connected = interface.readFrom(DRUM_ADDR, data, numPackets * PACKET_SIZE, true);
            midiInterface.parsePacket(data, numPackets * PACKET_SIZE);
        }
    }
};