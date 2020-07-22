#!/usr/bin/env python
import hid
h = hid.device()
h.open(0x1209, 0x2882)
h.send_feature_report([0x00,52])