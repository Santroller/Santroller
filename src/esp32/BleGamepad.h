#ifndef ESP32_BLE_GAMEPAD_H
#define ESP32_BLE_GAMEPAD_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BleConnectionStatus.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"

class BleGamepad
{
private:

    BleConnectionStatus *connectionStatus;

    NimBLEHIDDevice *hid;
    NimBLECharacteristic *inputGamepad;

    void rawAction(uint8_t msg[], char msgSize);
    static void taskServer(void *pvParameter);

public:
    BleGamepad(uint8_t batteryLevel = 100);
    void begin();
    void end(void);
    void sendReport(void* report, uint8_t len);
    bool isConnected(void);
    void setBatteryLevel(uint8_t level);
    uint8_t batteryLevel;

protected:
    virtual void onStarted(NimBLEServer *pServer){};
};

#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_GAMEPAD_H
