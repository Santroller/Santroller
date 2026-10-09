#pragma once
// Fake tusb.h: the GIP sources only need the common types and the device / host endpoint calls,
// which support/gip/fakes.cpp records
#include "common/tusb_common.h"
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "host/usbh.h"
