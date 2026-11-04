#include "devices/mic_stream.hpp"
#include <string.h>

namespace
{
// About 40ms at 48kHz
constexpr uint32_t RING_SIZE = 2048;
constexpr uint32_t RING_MASK = RING_SIZE - 1;
// How far behind the microphone reads are kept, enough to ride out a slow main loop
constexpr uint32_t TARGET_MS = 6;

int16_t s_ring[RING_SIZE];
// Free running sample counts, the ring holds write - read of them
uint32_t s_write;
uint32_t s_read;
// Position between s_read and the sample after it, in 1/65536ths of a sample
uint32_t s_phase;
uint32_t s_rate;
// Out of samples, so silence until the buffer has refilled to the target
bool s_starved = true;
MicStream::Stats s_stats;
}

void MicStream::set_source_rate(uint32_t rate)
{
    s_rate = rate;
    s_write = 0;
    s_read = 0;
    s_phase = 0;
    s_starved = true;
}

uint32_t MicStream::source_rate()
{
    return s_rate;
}

void MicStream::push(const int16_t *samples, size_t count)
{
    if (!s_rate)
    {
        return;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (s_write - s_read >= RING_SIZE)
        {
            // The emulated microphone isn't keeping up (or isn't streaming), keep the newest audio
            s_read++;
            s_stats.overruns++;
        }
        s_ring[s_write++ & RING_MASK] = samples[i];
        int16_t level = samples[i] < 0 ? (samples[i] == INT16_MIN ? INT16_MAX : -samples[i]) : samples[i];
        if (level > s_stats.peak)
        {
            s_stats.peak = level;
        }
    }
    s_stats.pushed += count;
}

void MicStream::gap()
{
    s_stats.gaps++;
}

void MicStream::read(int16_t *out, size_t count, uint32_t rate)
{
    memset(out, 0, count * sizeof(*out));
    if (!s_rate || !rate)
    {
        return;
    }
    const int32_t target = s_rate * TARGET_MS / 1000;
    int32_t fill = (int32_t)(s_write - s_read);
    if (s_starved)
    {
        if (fill < target)
        {
            return;
        }
        s_starved = false;
        s_phase = 0;
    }
    if (fill > target * 3)
    {
        // Far more buffered than the rate correction could ever work through, eg the microphone filled the
        // ring before the host started streaming, or the main loop stalled. Skip ahead to keep latency down.
        s_read = s_write - target;
        s_phase = 0;
        fill = target;
    }
    // Source samples per output sample, then sped up or slowed down by 0.1% per ms the buffer is off
    // target, up to 0.5%. Clock differences between a microphone and a console are well under that.
    const int32_t base = (int32_t)(((uint64_t)s_rate << 16) / rate);
    int32_t correction = (int32_t)((int64_t)base * (fill - target) / s_rate);
    if (correction > base / 200)
    {
        correction = base / 200;
    }
    else if (correction < -base / 200)
    {
        correction = -base / 200;
    }
    const uint32_t step = (uint32_t)(base + correction);
    for (size_t i = 0; i < count; i++)
    {
        if ((int32_t)(s_write - s_read) < 2)
        {
            s_starved = true;
            s_stats.underruns++;
            return;
        }
        // Linear interpolation, which is plenty for a voice into a pitch detector
        int32_t a = s_ring[s_read & RING_MASK];
        int32_t b = s_ring[(s_read + 1) & RING_MASK];
        // 15 bits of phase, so the product fits in 32 bits
        out[i] = (int16_t)(a + (((b - a) * (int32_t)(s_phase >> 1)) >> 15));
        s_phase += step;
        s_read += s_phase >> 16;
        s_phase &= 0xFFFF;
    }
}

MicStream::Stats MicStream::stats()
{
    Stats stats = s_stats;
    s_stats.peak = 0;
    return stats;
}
