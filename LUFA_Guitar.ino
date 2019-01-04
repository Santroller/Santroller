extern "C" {
  #include "XInputPad.h"
  #include "util.h"
}
#include "WiiExtension.h"
#include <EEPROM.h>
#include "MPU6050_6Axis_MotionApps20.h"
WiiExtension controller;
volatile bool mpuHasInterrupted = false;
MPU6050 mpu;
bool blinkState = false;
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
ExtensionPort port;
void mpuInterrupt() {
  mpuHasInterrupted = true;
}
void bootloader() {
  uint16_t *const bootKeyPtr = (uint16_t *)0x0800;

  // Value used by Caterina bootloader use to determine whether to run the
  // sketch or the bootloader programmer.
  uint16_t bootKey = 0x7777;

  *bootKeyPtr = bootKey;

  // setup watchdog timeout
  wdt_enable(WDTO_60MS);

  while(1) {} // wait for watchdog timer to trigger
}
void setup()
{
  setupMPU();
  xbox_init(true);
  // Disable JTAG
  bit_set(MCUCR, 1 << JTD);
  bit_set(MCUCR, 1 << JTD);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), mpuInterrupt, RISING);
  if (!port.update()) {
      port.connect();
  }
}
void loop()
{
  updateMPU();
  xbox_reset_watchdog();
  
  if (controller.read_controller((WiiController*)&gamepad_state, ypr)) {
    bootloader();
  }
  xbox_send_pad_state();
}

void updateMPU() {
   // if programming failed, don't try to do anything
    if (!dmpReady || !mpuHasInterrupted) return;

    // reset interrupt flag and get INT_STATUS byte
    mpuHasInterrupted = false;
    mpuIntStatus = mpu.getIntStatus();

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT))) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        fifoCount = mpu.getFIFOCount();

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        fifoCount -= packetSize;

        // display Euler angles in degrees
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    }
}

void setupMPU() {
    
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();
//    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
    
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);

    // verify connection
    mpu.testConnection();
    devStatus = mpu.dmpInitialize();
    
    mpu.setXAccelOffset(EEPROM.read(0));
    mpu.setYAccelOffset(EEPROM.read(1));
    mpu.setZAccelOffset(EEPROM.read(2));
    mpu.setXGyroOffset(EEPROM.read(3));
    mpu.setYGyroOffset(EEPROM.read(4));
    mpu.setZGyroOffset(EEPROM.read(5));
    mpu.setMultiMasterEnabled(true);
    mpu.setI2CBypassEnabled(true);
    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        mpu.setDMPEnabled(true);
        mpuIntStatus = mpu.getIntStatus();
        dmpReady = true;
       
        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } 

    // configure LED for output
    pinMode(LED_PIN, OUTPUT);
}
