#include "BleConnectionStatus.h"

BleConnectionStatus::BleConnectionStatus(void)
{
}

void BleConnectionStatus::onConnect(NimBLEServer *pServer, ble_gap_conn_desc* desc)
{
    pServer->updateConnParams(desc->conn_handle, 6, 25, 0, 6000);
    this->connected = true;
}

void BleConnectionStatus::onDisconnect(NimBLEServer *pServer)
{
    this->connected = false;
}
