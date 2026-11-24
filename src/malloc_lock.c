#include <reent.h>
#include "pico.h"
#include "hardware/sync.h"

// newlib's malloc lock, replacing the SDK's malloc mutex (PICO_USE_MALLOC_MUTEX=0).
//
// BTstack runs from an IRQ on core 0 (the CYW43 async context), and its callbacks allocate: the
// Wiimote emulator's report queue, our TLV store, pairing state. With the SDK mutex, an IRQ that
// allocates while core 0's main loop is inside malloc waits forever for a mutex its own core holds.
// Masking interrupts while the heap is locked keeps that IRQ out until malloc is done, and the
// spinlock still keeps the other core out. Recursive, as newlib's realloc calls malloc with it held.
static int8_t s_owner = -1;
static uint32_t s_depth;
static uint32_t s_saved_irq;

static inline spin_lock_t *malloc_spin_lock(void)
{
    return spin_lock_instance(PICO_SPINLOCK_ID_OS1);
}

void __malloc_lock(struct _reent *r)
{
    (void)r;
    int8_t core = (int8_t)get_core_num();
    // only this core ever stores its own number here, and it does so with interrupts off
    if (s_owner == core)
    {
        s_depth++;
        return;
    }
    uint32_t saved = spin_lock_blocking(malloc_spin_lock());
    s_owner = core;
    s_depth = 1;
    s_saved_irq = saved;
}

void __malloc_unlock(struct _reent *r)
{
    (void)r;
    if (--s_depth)
        return;
    s_owner = -1;
    spin_unlock(malloc_spin_lock(), s_saved_irq);
}
