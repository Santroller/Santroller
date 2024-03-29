import serial
from collections import namedtuple
ser = serial.Serial("/dev/ttyACM0", 1000000)
MAGIC = 0x14
READY = 0x65
RESET = 0x73
DESCRIPTOR = 0x77
CONTROL_REQUEST = 0x78
USB_DESCRIPTOR_DEVICE = 0x01
USB_DESCRIPTOR_CONFIGURATION = 0x02
USB_DESCRIPTOR_STRING = 0x03
HID_DESCRIPTOR_REPORT = 0x22
DeviceDescriptor = namedtuple('DeviceDescriptor', 'size type version devClass devSubclass devProtocol endpoint0size vid pid release manufacturer_index product_index serial_index')
ready = ser.read(1)[0] == READY
if not ready:
    ready = ser.read(1)[0] == READY
print(f"Can see 328p = {ready}")

def receivePacket():
    header = ser.read(3)
    magic = header[0]
    id = header[1]
    len = header[2]
    if magic != MAGIC:
        print("Invalid packet recieved!")
        print(header)
        exit(1)
    return (id, ser.read(len))


def receiveDescriptor(number, type, length):
    command = b"\x14\x77\x09"
    command += number.to_bytes(1, 'little')
    command += type.to_bytes(1, 'little')
    command += (0).to_bytes(2, 'little')
    command += length.to_bytes(2, 'little')
    ser.write(command)
    ret = receivePacket()
    if ret[0] == RESET:
        print("Reset requested!")
        return b""
    if ret[0] != DESCRIPTOR:
        print("Incorrect packet recieved!")
    return ret[1][2:]

def sendControlRequest(requestType, request, wValue, wIndex, wLength, data=None):
    command = b"\x14\x78\x0B"
    command += requestType.to_bytes(1, 'little')
    command += request.to_bytes(1, 'little')
    command += wValue.to_bytes(2, 'little')
    command += wIndex.to_bytes(2, 'little')
    command += wLength.to_bytes(2, 'little')
    if data:
        command += data
    ser.write(command)
    ret = receivePacket()
    return ret


print(receiveDescriptor(0x01, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(receiveDescriptor(0x02, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(receiveDescriptor(0x03, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(sendControlRequest(192, 32, 0 , 4, 64))
print(receiveDescriptor(0x01, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(receiveDescriptor(0x01, HID_DESCRIPTOR_REPORT, 178))
print(receiveDescriptor(0x01, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(receiveDescriptor(0x02, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
print(receiveDescriptor(0x03, USB_DESCRIPTOR_STRING, 178).decode("utf-16"))
