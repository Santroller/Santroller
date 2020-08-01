#!/usr/bin/env python
import time
import hid
h = hid.device()
h.open(0x1209, 0x2882)
h.send_feature_report([0x00,58])
time.sleep(1)
print(bytes(h.get_feature_report(0x00,0x20)).decode('utf-8'))