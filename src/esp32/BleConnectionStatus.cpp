#include "BleConnectionStatus.h"
#include "NimBLEDevice.h"

BleConnectionStatus::BleConnectionStatus(void)
{
}

void BleConnectionStatus::onConnect(NimBLEServer *pServer, ble_gap_conn_desc* desc)
{
    pServer->updateConnParams(desc->conn_handle, 6, 25, 0, 6000);
    this->connected = true;
    BLEDescriptor *bdesc = this->inputGamepad->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    uint8_t val[] = {0x01, 0x00};
    bdesc->setValue(val, 2);
}

void BleConnectionStatus::onDisconnect(NimBLEServer *pServer)
{
    this->connected = false;
}
