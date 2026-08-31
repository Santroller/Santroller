/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "xgip_protocol.h"
#include <string.h>
#include <stdio.h>

// Default Constructor
XGIPProtocol::XGIPProtocol()
{
    reset();
}

// Default Destructor
XGIPProtocol::~XGIPProtocol()
{
}

void XGIPProtocol::copyAttributes(XGIPProtocol* packet)
{
    reset();
    header = packet->header;
    setData(packet->getData(), packet->getDataLength());
}

// Reset packet information
void XGIPProtocol::reset()
{
    memset((void *)&header, 0, sizeof(GipHeader_t));
    actualDataReceived = 0; // How much actual data have we received?
    totalDataSent = 0;      // How much actual data have we sent?
    numberOfChunksSent = 0; // How many actual chunks have we sent?
    chunkEnded = false;     // Are we at the end of the chunk?
    isValidPacket = false;  // Is this a valid packet?
    memset(data, 0, 1024);
    dataLength = 0;                    // Set data length to 0
    memset(packet, 0, sizeof(packet)); // Set our packet to 0
    packetLength = 0;                  // Set packet length to 0
    isWaitingToSend = true;
}

// Parse incoming packet
bool XGIPProtocol::parse(const uint8_t *buffer, uint16_t len)
{
    // Do we have enough room for a header? No, this isn't valid
    if (len < 4)
    {
        reset();
        isValidPacket = false;
        return false;
    }

    // Set packet length
    packetLength = len;

    // Use buffer as a raw packet without copying to our internal structure
    GipHeader_t *newPacket = (GipHeader_t *)buffer;
    if (newPacket->command == GIP_ACK_RESPONSE)
    {
        if (len != 13 || newPacket->internal != 0x01 || newPacket->length != 0x09)
        {
            reset();
            isValidPacket = false;
            return false; // something malformed in this packet
        }
        memcpy((void *)&header, buffer, sizeof(GipHeader_t));
        isValidPacket = true; // don't do anything with ack packets for now
        return true;
    }
    else
    { // Non-ACK
        // Continue parsing chunked data
        if (newPacket->chunked == true)
        {
            memcpy((void *)&header, buffer, sizeof(GipHeader_t)); // Always copy to header buffer
            uint16_t packet_len;
            uint16_t total_len_or_offset;
            const uint8_t *packet = buffer + offsetof(GipHeader_t, length);
            packet += readLeb128(packet, &packet_len);
            packet += readLeb128(packet, &total_len_or_offset);
            if (packet_len == 0)
            { // END OF CHUNK
                // Verify chunk is good
                if (dataLength != total_len_or_offset)
                {
                    isValidPacket = false;
                    return false;
                }
                chunkEnded = true;
                isValidPacket = true;
                return true; // we're good!
            }
            if (header.chunkStart == 1)
            { // START OF CHUNK
                reset();
                memcpy((void *)&header, buffer, sizeof(GipHeader_t));
                dataLength = total_len_or_offset;
            }
            if (packet_len > sizeof(data) - actualDataReceived || packet + packet_len > buffer + len)
            {
                isValidPacket = false;
                return false;
            }
            memcpy(&data[actualDataReceived], packet, packet_len); //
            actualDataReceived += packet_len;
            numberOfChunksSent++; // count our chunks for the ACK
            isValidPacket = true;
        }
        else
        {
            reset();
            memcpy((void *)&header, buffer, sizeof(GipHeader_t));
            if (header.length > 0)
            {
                if (sizeof(GipHeader_t) + header.length > len)
                {
                    reset();
                    isValidPacket = false;
                    return false;
                }
                memcpy(data, &buffer[4], header.length); // copy incoming data
            }
            actualDataReceived = header.length;
            dataLength = actualDataReceived;
            isValidPacket = true;
            return true;
        }
    }

    return false;
}

bool XGIPProtocol::endOfChunk()
{
    return chunkEnded;
}

bool XGIPProtocol::validate()
{ // is valid packet?
    return isValidPacket;
}

void XGIPProtocol::incrementSequence()
{
    header.sequence++;
    if (header.sequence == 0)
        header.sequence = 1;
}

void XGIPProtocol::setAttributes(uint8_t cmd, uint8_t seq, uint8_t internal, uint8_t isChunked, uint8_t needsAck)
{ // Set attributes for next output packet
    header.command = cmd;
    header.sequence = seq;
    header.internal = internal;
    header.chunked = isChunked;
    header.needsAck = needsAck;
}

bool XGIPProtocol::setData(const uint8_t *buffer, uint16_t len)
{
    if (len > 0x3000)
    { // arbitrary but this should cover us if something bad happens
        return false;
    }
    memcpy(data, buffer, len);
    dataLength = len;
    return true;
}

uint8_t XGIPProtocol::readLeb128(const uint8_t *data, uint16_t *out)
{
    uint8_t read = 0;
    uint16_t result = 0;
    uint16_t shift = 0;
    unsigned char byte;
    do
    {
        byte = *data++;
        result |= (byte & 0x7f) << shift; /* low-order 7 bits of byte */
        shift += 7;
        read++;
    } while ((byte & 0x80) != 0);
    *out = result;
    return read;
}

uint16_t XGIPProtocol::writeLeb128(uint8_t *dest, uint16_t len, bool pad)
{
    if (pad)
    {
        // if < 127, length is put in the second byte
        if (len < 0x7f)
        {
            dest[0] = 0x80 | len;
            dest[1] = 0;
            return 2;
        }
    }
    uint16_t temp = 0;
    do
    {
        uint8_t byte = len & 0x7f; /* low-order 7 bits of value */
        len >>= 7;
        if (len != 0)     /* more bytes to come */
            byte |= 0x80; /* set high-order bit of byte */
        *dest++ = byte;
        temp++;
    } while (len != 0);
    return temp;
}
// Generate XGIP Packet for output
uint8_t *XGIPProtocol::generatePacket()
{
    if (header.chunked == 0)
    { // Simple data packet does not require chunk logic
        header.length = (uint8_t)dataLength;
        memcpy(packet, &header, sizeof(GipHeader_t));
        memcpy((void *)&packet[4], data, dataLength);
        packetLength = sizeof(GipHeader_t) + dataLength;
        isWaitingToSend = false;
    }
    else
    { // Are we a chunk?
        if (numberOfChunksSent > 0 && totalDataSent == dataLength)
        { // General Final Chunk Packet (End-Packet)
            header.needsAck = 0;
            header.length = 0;
            memcpy(packet, &header, sizeof(GipHeader_t));
            packetLength = sizeof(GipHeader_t) + writeLeb128(packet + sizeof(GipHeader_t), dataLength, true);
            chunkEnded = true;
            isWaitingToSend = false;
        }
        else
        {
            if (numberOfChunksSent == 0)
            {
                if (dataLength < GIP_MAX_CHUNK_SIZE)
                {
                    // In the rare case the chunked packet is < max chunk size
                    // we set the chunk flags to 0, set our actual data length
                    // BUT we still require an ACK and have to reply to it
                    header.chunkStart = 0;
                    header.chunked = 0;
                }
                else
                {
                    header.chunkStart = 1;
                }
            }
            else
            {
                header.chunkStart = 0; // set chunk start to 0 in all other cases
            }

            // Ack on 1st and every 5th interval
            //    Note: this will send on (0 chunks sent) 1st, (4 chunks sent) 5th, (5 chunks sent) 10th, (5 chunks sent) 15th. this is correct
            if (numberOfChunksSent == 0 || (numberOfChunksSent + 1) % 5 == 0)
            {
                header.needsAck = 1;
            }
            else
            {
                header.needsAck = 0;
            }

            // Assume we're sending the maximum chunk size
            uint16_t dataToSend = GIP_MAX_CHUNK_SIZE;
            bool end = false;

            // If we're at the end, reduce data to send and set the ack flag
            if ((dataLength - totalDataSent) < dataToSend)
            {
                dataToSend = dataLength - totalDataSent;
                header.needsAck = 1;
                end = true;
            }

            uint8_t *lebPacket = packet + offsetof(GipHeader_t, length);
            // Copy our header and data to the packet
            memcpy(packet, &header, sizeof(GipHeader_t));
            if (numberOfChunksSent == 0)
            {
                lebPacket += writeLeb128(lebPacket, dataToSend, dataLength < 0x80);
                lebPacket += writeLeb128(lebPacket, dataLength, false);
            }
            else
            {
                lebPacket += writeLeb128(lebPacket, dataToSend, totalDataSent < 0x80 && !end);
                lebPacket += writeLeb128(lebPacket, totalDataSent, false);
            }
            memcpy(lebPacket, &data[totalDataSent], dataToSend);
            packetLength = lebPacket - packet + dataToSend;
            totalDataSent += dataToSend; // Total Data Sent in bytes
            numberOfChunksSent++;        // Number of Chunks sent so far
        }
    }
    return packet;
}

uint8_t *XGIPProtocol::generateAckPacket()
{ // Generate output packet
    packet[0] = 0x01;
    packet[1] = 0x20;
    packet[2] = header.sequence;
    packet[3] = 0x09;
    packet[4] = 0x00;
    packet[5] = header.command;
    packet[6] = 0x20;

    // we have to keep track of # of chunks because data received for ACK is +2 for size of chunk
    uint16_t dataReceived = actualDataReceived;
    packet[7] = dataReceived & 0x00FF;
    packet[8] = (dataReceived & 0xFF00) >> 8;
    packet[9] = 0x00;
    packet[10] = 0x00;
    if (header.chunked == true)
    { // Are we a chunk?
        uint16_t left = dataLength - dataReceived;
        packet[11] = left & 0x00FF;
        packet[12] = (left & 0xFF00) >> 8;
    }
    else
    {
        packet[11] = 0;
        packet[12] = 0;
    }
    packetLength = 13;
    return packet;
}

// Get last generated output packet length
uint16_t XGIPProtocol::getPacketLength()
{
    return packetLength;
}

// Get the header information if the packet needs an ACK
uint8_t XGIPProtocol::getPacketAck()
{
    return header.needsAck;
}

// Get command of a parsed packet
uint8_t XGIPProtocol::getCommand()
{
    return header.command;
}

// Is this packet chunked?
uint8_t XGIPProtocol::getChunked()
{
    return header.chunked;
}

// Get seqeuence in the header
uint8_t XGIPProtocol::getSequence()
{
    return header.sequence;
}

// Get data from a packet or packet-chunk
uint8_t *XGIPProtocol::getData()
{
    return data;
}

// Get length of a packet or packet-chunk
uint16_t XGIPProtocol::getDataLength()
{
    return dataLength;
}

// Get chunk data from incoming packet
bool XGIPProtocol::getChunkData(XGIPProtocol &packet)
{
    return false;
}

// Last packet parsed needs an ACK
bool XGIPProtocol::ackRequired()
{
    return header.needsAck;
}

bool XGIPProtocol::waitingToSend()
{
    return isWaitingToSend;
}