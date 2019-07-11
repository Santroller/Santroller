#include "../../shared/output/output_serial.h"
#include "lightweight_ring_buffer.h"
/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};

/** Circular buffer to hold data from the host before it is sent to the device
 * via the serial port. */
RingBuff_t USBtoUSART_Buffer;

/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuff_t USARTtoUSB_Buffer;

void serial_configuration_changed() {
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void serial_control_request() {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void serial_init() {
  RingBuffer_InitBuffer(&USBtoUSART_Buffer);
  RingBuffer_InitBuffer(&USARTtoUSB_Buffer);
}
void serial_tick() {
  /* Only try to read in bytes from the CDC interface if the transmit buffer is
   * not full */
  if (!(RingBuffer_IsFull(&USBtoUSART_Buffer))) {
    int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

    /* Read bytes from the USB OUT endpoint into the USART transmit buffer */
    if (!(ReceivedByte < 0))
      RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
  }

  /* Check if the UART receive buffer flush timer has expired or the buffer is
   * nearly full */
  RingBuff_Count_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
  if ((TIFR0 & (1 << TOV0)) || (BufferCount > BUFFER_NEARLY_FULL)) {
    TIFR0 |= (1 << TOV0);
    /* Read bytes from the USART receive buffer into the USB IN endpoint */
    while (BufferCount--)
      CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                          RingBuffer_Remove(&USARTtoUSB_Buffer));
  }

  /* Load the next byte from the USART transmit buffer into the USART */
  if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer))) {
    Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
  }

  CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
}

void serial_receive(uint8_t ReceivedByte) {
  if (USB_DeviceState == DEVICE_STATE_Configured)
    RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}