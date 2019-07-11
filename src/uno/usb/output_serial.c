#include "../../shared/output/output_serial.h"
#include "lightweight_ring_buffer.h"
#include <LUFA/Drivers/Board/Board.h>
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
  /* Start the flush timer so that overflows occur rapidly to push received
   * bytes to the USB interface */
  TCCR0B = (1 << CS02);
  Serial_Init(57600, false);
  /* Pull target /RESET line high */
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
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
/** Event handler for the CDC Class driver Host-to-Device Line Encoding Changed
 * event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  bool CurrentDTRState =
      (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
       CDC_CONTROL_LINE_OUT_DTR);
  // At this point, we know if we are in programming mode or not
  if (CurrentDTRState)
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
  else
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
}
// In theory, we would want this. however, being able to change speeds actually makes things more difficult for us, so we could just not allow that at all.
// /** Event handler for the CDC Class driver Line Encoding Changed event.
//  *
//  *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
//  */
// void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
// {
// 	uint8_t ConfigMask = 0;

// 	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
// 	{
// 		case CDC_PARITY_Odd:
// 			ConfigMask = ((1 << UPM11) | (1 << UPM10));		
// 			break;
// 		case CDC_PARITY_Even:
// 			ConfigMask = (1 << UPM11);		
// 			break;
// 	}

// 	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
// 	  ConfigMask |= (1 << USBS1);

// 	switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
// 	{
// 		case 6:
// 			ConfigMask |= (1 << UCSZ10);
// 			break;
// 		case 7:
// 			ConfigMask |= (1 << UCSZ11);
// 			break;
// 		case 8:
// 			ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
// 			break;
// 	}

// 	/* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
// 	UCSR1B = 0;
// 	UCSR1A = 0;
// 	UCSR1C = 0;

// 	/* Special case 57600 baud for compatibility with the ATmega328 bootloader. */	
// 	UBRR1  = (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 57600)
// 			 ? SERIAL_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS)
// 			 : SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);	

// 	UCSR1C = ConfigMask;
// 	UCSR1A = (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 57600) ? 0 : (1 << U2X1);
// 	UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
// }