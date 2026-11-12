#include "bootsel_pairing.h"
#include <stdio.h>
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/timer.h"
#include "hardware/sync.h"
#include "commands.pb.h"
#include "utils.h"
#include "emulation/usb/hid_device.h"
#include "managers/device_manager.hpp"
#include "managers/inactivity_manager.hpp"
#include "managers/profile_manager.hpp"

// BOOTSEL pulls the flash's CS low, so reading it means taking CS from the flash for a moment. Nothing can run
// from flash meanwhile: this runs from RAM with interrupts off, and core1 only ever runs from RAM
// (flash_core1_loop). Kept short, as PIO-USB host frames are timed from interrupts.
static bool __no_inline_not_in_flash_func(read_bootsel)()
{
    const uint CS_PIN_INDEX = 1;
    uint32_t flags = save_and_disable_interrupts();
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl, GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    // The pull up only has the pin's capacitance to charge, so a few us is plenty for it to settle
    uint32_t start = timer_hw->timerawl;
    while (timer_hw->timerawl - start < 5)
    {
    }
#if PICO_RP2040
    bool pressed = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));
#else
    bool pressed = !(sio_hw->gpio_hi_in & SIO_GPIO_HI_IN_QSPI_CSN_BITS);
#endif
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl, GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    restore_interrupts(flags);
    return pressed;
}

static constexpr uint32_t POLL_INTERVAL_MS = 20;

void bootsel_pairing_update()
{
    static uint32_t last_poll = 0;
    // Starts out pressed, so a button held when this starts watching doesn't count until it's let go
    static bool was_pressed = true;
    uint32_t now = millis();
    if (now - last_poll < POLL_INTERVAL_MS)
    {
        return;
    }
    last_poll = now;
    // Reading BOOTSEL stalls everything for a moment, which only doesn't matter while nothing is active. The tool
    // has its own pair button.
    if (ProfileManager::instance().has_active_instances() || HIDConfigDevice::tool_may_be_open())
    {
        was_pressed = true;
        return;
    }
    bool pressed = read_bootsel();
    bool edge = pressed && !was_pressed;
    was_pressed = pressed;
    if (!edge)
    {
        return;
    }
    printf("bootsel: pairing\r\n");
    InactivityManager::instance().input_activity(now);
    proto_Command command = proto_Command_init_zero;
    command.which_command = proto_Command_scan_tag;
    DeviceManager::instance().for_each_active_device([&command](const auto &device) {
        device->handle_command(command);
    });
}
