#ifdef INPUT_USB_HOST
uint8_t device_count = get_usb_host_device_count();
bool poke_ghl = false;

if (millis() - lastSentGHLPoke > 8000) {
    poke_ghl = true;
    lastSentGHLPoke = millis();
}
USB_Host_Data_t usb_host_data;
memset(&usb_host_data, 0, sizeof(usb_host_data));
// Slider rests at 0x80
usb_host_data.slider = 0x80;
for (int i = 0; i < device_count; i++) {
    USB_Device_Type_t device_type = get_usb_host_device_type(i);
    // Midi gets handled async
    if (device_type.console_type == MIDI_ID) {
        continue;
    }
    // Poke any GHL guitars to keep em alive
    if (poke_ghl && device_type.sub_type == LIVE_GUITAR) {
        if (device_type.console_type == PS3) {
            transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps3wiiu_magic_data), ghl_ps3wiiu_magic_data);
        } else if (device_type.console_type == PS4) {
            transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps4_magic_data), ghl_ps4_magic_data);
        } else if (device_type.console_type == XBOXONE) {
            if (ghl_sequence_number_host == 0) {
                ghl_sequence_number_host = 1;
            }
            XboxOneGHLGuitar_Output_t data;
            XboxOneGHLGuitar_Output_t *report = &data;
            GIP_HEADER(report, GHL_HID_OUTPUT, false, ghl_sequence_number_host++);
            data.sub_command = 0x02;
            data.data[0] = 0x08;
            data.data[1] = 0x0A;
            send_report_to_controller(device_type.dev_addr, device_type.instance, (uint8_t *)&data, sizeof(data));
        }
    }
    uint8_t *data = (uint8_t *)&temp_report_usb_host;
    uint8_t len = get_usb_host_device_data(i, data);
    uint8_t console_type = device_type.console_type;
    if (console_type == XBOXONE) {
        GipHeader_t *header = (GipHeader_t *)data;
        if (device_type.sub_type == LIVE_GUITAR && header->command == GHL_HID_REPORT) {
            // Xbox one GHL guitars actually end up using PS3 reports if you poke them.
            console_type = PS3;
            data = (uint8_t *)&((XboxOneGHLGuitar_Data_t *)data)->report;
        } else if (header->command != GIP_INPUT_REPORT) {
            // Not input data, continue
            continue;
        }
    }
    convert_report(data, len, device_type, &usb_host_data);
}
memcpy(&last_usb_host_data, &usb_host_data, sizeof(usb_host_data));
#endif
