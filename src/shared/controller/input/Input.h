#pragma once
#include <stdlib.h>
class Input {
public:
  virtual void init() = 0;
  virtual void read_controller(Controller *controller) = 0;
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *ptr) { free(ptr); }
};