#pragma once
#include <stdint.h>
#include <stddef.h>

// Mono 16 bit audio from a USB microphone on the host port, on its way to an emulated microphone.
// Both sides run from the main loop, so nothing here is locked.
namespace MicStream
{
    // A microphone started streaming at this rate, or stopped (0)
    void set_source_rate(uint32_t rate);
    uint32_t source_rate();
    void push(const int16_t *samples, size_t count);
    // Packets the microphone sent that were lost on the way in
    void gap();

    // Fills out with count samples at rate, resampled from the source, or silence without one.
    // Called once per frame, and drifts its read speed to keep the buffer from over or underrunning,
    // as the microphone and the console run off different clocks
    void read(int16_t *out, size_t count, uint32_t rate);

    struct Stats
    {
        uint32_t pushed;
        uint32_t gaps;
        uint32_t overruns;
        uint32_t underruns;
        int16_t peak;
    };
    // Counters for debugging, the peak is reset by each call
    Stats stats();
}
