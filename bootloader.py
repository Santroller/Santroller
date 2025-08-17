#!/usr/bin/env python3
import hid

vid = 0x1209
pid = 0x2882
try:
	with hid.Device(vid, pid) as h:
		print(f'Device manufacturer: {h.manufacturer}')
		print(f'Product: {h.product}')
		print(f'Serial Number: {h.serial}')
		h.send_feature_report(b"\x26\0")
except:
	pass