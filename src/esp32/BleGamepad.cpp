#include "BleGamepad.h"

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <driver/adc.h>

#include "BleConnectionStatus.h"
#include "HIDKeyboardTypes.h"
#include "HIDTypes.h"
#include "NimBLEHIDDevice.h"
#include "config.h"
#include "endpoints.h"
#include "hid.h"
#include "sdkconfig.h"
#include "shared_main.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "BLEGamepad"
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLEGamepad";
#endif

#define SERVICE_UUID_DEVICE_INFORMATION "180A"  // Service - Device information

#define CHARACTERISTIC_UUID_MODEL_NUMBER "2A24"       // Characteristic - Model Number String - 0x2A24
#define CHARACTERISTIC_UUID_SOFTWARE_REVISION "2A28"  // Characteristic - Software Revision String - 0x2A28
#define CHARACTERISTIC_UUID_SERIAL_NUMBER "2A25"      // Characteristic - Serial Number String - 0x2A25
#define CHARACTERISTIC_UUID_FIRMWARE_REVISION "2A26"  // Characteristic - Firmware Revision String - 0x2A26
#define CHARACTERISTIC_UUID_HARDWARE_REVISION "2A27"  // Characteristic - Hardware Revision String - 0x2A27

uint8_t tempHidReportDescriptor[150];
int hidReportDescriptorSize = 0;
uint8_t reportSize = 0;
uint8_t numOfButtonBytes = 0;
uint16_t vid;
uint16_t pid;
uint16_t axesMin;
uint16_t axesMax;
uint16_t simulationMin;
uint16_t simulationMax;
std::string modelNumber;
std::string softwareRevision;
std::string serialNumber;
std::string firmwareRevision;
std::string hardwareRevision;

BleGamepad::BleGamepad(uint8_t batteryLevel) : hid(0) {
    this->batteryLevel = batteryLevel;
    this->connectionStatus = new BleConnectionStatus();
}

void BleGamepad::begin() {
    modelNumber = "";
    serialNumber = "";
    softwareRevision = "";
    firmwareRevision = "";
    hardwareRevision = "";

    vid = ARDWIINO_VID;
    pid = ARDWIINO_PID_BLE;

    xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleGamepad::end(void) {
}

bool BleGamepad::isConnected(void) {
    return this->connectionStatus->connected;
}

void BleGamepad::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    if (hid != 0) {
        this->hid->setBatteryLevel(this->batteryLevel);

        if (this->isConnected()) {
            this->hid->batteryLevel()->notify();
        }
    }
}

void BleGamepad::sendReport(void* report, uint8_t len) {
    this->inputGamepad->setValue((uint8_t*)report, len);
    this->inputGamepad->notify();
}

void BleGamepad::taskServer(void *pvParameter) {
    BleGamepad *BleGamepadInstance = (BleGamepad *)pvParameter;  // static_cast<BleGamepad *>(pvParameter);

    // Use the procedure below to set a custom Bluetooth MAC address
    // Compiler adds 0x02 to the last value of board's base MAC address to get the BT MAC address, so take 0x02 away from the value you actually want when setting
    // uint8_t newMACAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF - 0x02};
    // esp_base_mac_addr_set(&newMACAddress[0]); // Set new MAC address

    NimBLEDevice::init("SantrollerBT");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(BleGamepadInstance->connectionStatus);

    BleGamepadInstance->hid = new NimBLEHIDDevice(pServer);

    BleGamepadInstance->inputGamepad = BleGamepadInstance->hid->inputReport(0);  // <-- input REPORTID from report map
    BleGamepadInstance->connectionStatus->inputGamepad = BleGamepadInstance->inputGamepad;

    BleGamepadInstance->hid->manufacturer()->setValue("sanjay900");

    NimBLEService *pService = pServer->getServiceByUUID(SERVICE_UUID_DEVICE_INFORMATION);

    BLECharacteristic *pCharacteristic_Model_Number = pService->createCharacteristic(
        CHARACTERISTIC_UUID_MODEL_NUMBER,
        NIMBLE_PROPERTY::READ);
    pCharacteristic_Model_Number->setValue(modelNumber);

    BLECharacteristic *pCharacteristic_Software_Revision = pService->createCharacteristic(
        CHARACTERISTIC_UUID_SOFTWARE_REVISION,
        NIMBLE_PROPERTY::READ);
    pCharacteristic_Software_Revision->setValue(softwareRevision);

    BLECharacteristic *pCharacteristic_Serial_Number = pService->createCharacteristic(
        CHARACTERISTIC_UUID_SERIAL_NUMBER,
        NIMBLE_PROPERTY::READ);
    pCharacteristic_Serial_Number->setValue(serialNumber);

    BLECharacteristic *pCharacteristic_Firmware_Revision = pService->createCharacteristic(
        CHARACTERISTIC_UUID_FIRMWARE_REVISION,
        NIMBLE_PROPERTY::READ);
    pCharacteristic_Firmware_Revision->setValue(firmwareRevision);

    BLECharacteristic *pCharacteristic_Hardware_Revision = pService->createCharacteristic(
        CHARACTERISTIC_UUID_HARDWARE_REVISION,
        NIMBLE_PROPERTY::READ);
    pCharacteristic_Hardware_Revision->setValue(hardwareRevision);

    BleGamepadInstance->hid->pnp(0x01, vid, pid, 0x0110);
    BleGamepadInstance->hid->hidInfo(0x00, 0x01);

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);

    uint8_t *customHidReportDescriptor = new uint8_t[hidReportDescriptorSize];
    memcpy(customHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);

    // Testing
    // for (int i = 0; i < hidReportDescriptorSize; i++)
    //    Serial.printf("%02x", customHidReportDescriptor[i]);

    
#if SUPPORTS_KEYBOARD
    BleGamepadInstance->hid->reportMap((uint8_t *)keyboard_mouse_descriptor, sizeof(keyboard_mouse_descriptor));
#else
    // setup HID Device service
    BleGamepadInstance->hid->reportMap((uint8_t *)pc_descriptor, sizeof(pc_descriptor));
#endif
    BleGamepadInstance->hid->startServices();

    BleGamepadInstance->onStarted(pServer);

    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_GAMEPAD);
    pAdvertising->addServiceUUID(BleGamepadInstance->hid->hidService()->getUUID());
    pAdvertising->start();
    BleGamepadInstance->hid->setBatteryLevel(BleGamepadInstance->batteryLevel);

    ESP_LOGD(LOG_TAG, "Advertising started!");
    vTaskDelay(portMAX_DELAY);  // delay(portMAX_DELAY);
}
