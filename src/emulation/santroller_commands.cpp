#include "emulation/santroller_commands.hpp"
#include "protocols/santroller_output.hpp"

bool santroller_handle_output_command(Instance &instance, const uint8_t *data, uint16_t len)
{
    if (len >= 2 && data[0] == ReportIdGamepad && santroller_is_command(data[1]))
    {
        data++;
        len--;
    }
    if (len < 1 || !santroller_is_command(data[0]))
    {
        return false;
    }
    switch (data[0])
    {
    case SANTROLLER_COMMAND_RUMBLE:
        if (len >= 3)
        {
            uint8_t left = data[1];
            uint8_t right = data[2];
            // same split as an Xbox 360 rumble report
            if (instance.subtype == DjHeroTurntable && left == right)
                instance.set_euphoria_led(left);
            else if (subtype_supports_stagekit(instance.subtype))
                instance.process_stagekit_command(right, left);
            else
                instance.set_rumble(left, right);
        }
        break;
    case SANTROLLER_COMMAND_PLAYER_LED:
        if (len >= 2)
            instance.set_player_led(data[1] < 4 ? data[1] + 1 : 0);
        break;
    case SANTROLLER_COMMAND_FEEDBACK:
        instance.process_feedback_report(data, len);
        break;
    case SANTROLLER_COMMAND_RGB_LED:
        if (len >= 4)
            instance.set_lightbar(data[1], data[2], data[3]);
        break;
    }
    return true;
}
