#pragma once
class GPIOInterface {
   public:
    GPIOInterface();
    void high();
    void low();
    void registerInterrupt(void (*)());
};