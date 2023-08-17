#pragma once

#include <stdbool.h>
#include "atomic.h"

/*

透過 atomic 去設定 state true 時為 lock，false 為 unlock，
在當要 lock 前，透過 atomic relaxed 去確認現在的 state 為 flase 後，再嘗試透過 atomic acquire 將 state 改為 true

*/

typedef struct {
    atomic bool state;
} spinlock_t;

#define SPINLOCK_INITIALIZER \
    {                        \
        .state = false       \
    }

static inline void spin_init(spinlock_t *lock)
{
    atomic_init(&lock->state, false);
}

static inline bool spin_trylock(spinlock_t *lock)
{
    /* Do a read first to avoid bouncing the cache line if it is locked */
    return !load(&lock->state, relaxed) &&
           !exchange(&lock->state, true, acquire);
}

/* FIXME: support more platforms */
#if !defined(__GNUC__) && !defined(__clang__)
#error "unsupported compilers"
#endif

#if defined(__i386__) || defined(__x86_64__)
#define spin_hint() __builtin_ia32_pause()
#elif defined(__aarch64__)
#define spin_hint() __asm__ __volatile__("isb\n")
#else
#define spin_hint() ((void) 0)
#endif

static inline void spin_lock(spinlock_t *lock)
{
    /* Given the specific implementation of spin_trylock(), this results in
     * a test-and-test-and-set (TTAS) loop. Refer to
     * https://rigtorp.se/spinlock/ for more information.
     */
    while (!spin_trylock(lock))
        spin_hint();
}

static inline void spin_unlock(spinlock_t *lock)
{
    store(&lock->state, false, release);
}