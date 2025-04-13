#include <stdbool.h>
#include <stdint.h>
class Core {
   public:
    static void go_to_sleep();
    static void read_serial(char* id, uint8_t len);
    static void delayMicroseconds(unsigned int usec);
    static void delay(unsigned long ms);
    static long micros();
    static long millis();
};