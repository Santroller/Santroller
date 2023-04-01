#ifdef INPUT_USB_HOST
    uint8_t device_count = get_usb_host_device_count();
    bool poke_ghl = false;

    if (millis() - lastSentGHLPoke > 8000) {
        poke_ghl = true;
        lastSentGHLPoke = millis();
    }
    for (int i = 0; i < device_count; i++) {
        USB_Device_Type_t device_type = get_usb_host_device_type(i);
        // Poke any GHL guitars to keep em alive
        if (poke_ghl && device_type.sub_type == LIVE_GUITAR) {
            if (device_type.console_type == PS3) {
                memcpy_P(buf, ghl_ps3wiiu_magic_data, sizeof(ghl_ps3wiiu_magic_data));
                transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps3wiiu_magic_data), (uint8_t *)buf);
            } else if (device_type.console_type == PS4) {
                memcpy_P(buf, ghl_ps4_magic_data, sizeof(ghl_ps4_magic_data));
                transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps4_magic_data), (uint8_t *)buf);
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
                send_report_to_controller(device_type.dev_addr, (uint8_t *)&data, sizeof(data));
            }
        }
    }
#endif