#pragma once

#if USE_PTHREADS

#include <pthread.h>
#include <stdio.h>

#define mutex_t pthread_mutex_t

#if FIX_PI

static inline void mutex_init(mutex_t *m) {
    // reference: https://www.embedded.com/effective-use-of-pthreads-in-embedded-linux-designs-part-2-sharing-resources/
    printf("pthread mutex\n");
    pthread_mutexattr_t attr;
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(m, &attr);
}

# else
#define mutex_init(m) pthread_mutex_init(m, NULL)
# endif

#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define mutex_trylock(m) (!pthread_mutex_trylock(m))
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock

#else

#include <stdbool.h>
#include "atomic.h"
#include "futex.h"
#include "spinlock.h"
#include <stdio.h>


/*
用 state 和 
*/

typedef struct {
    atomic int state;
} mutex_t;

enum {
    MUTEX_LOCKED = 1 << 0,
    MUTEX_SLEEPING = 1 << 1,
};

#define MUTEX_INITIALIZER \
    {                     \
        .state = 0        \
    }

static inline void mutex_init(mutex_t *mutex)
{
    printf("customer mutex\n");
    atomic_init(&mutex->state, 0);
}

// 嘗試上鎖，首先會取得現在的狀態，如果是上鎖狀態返回 false，接著嘗試上鎖後，判斷狀態，如果不是上鎖，返回 false，接著設定 thread_fence 去防止指令重新排列
static bool mutex_trylock(mutex_t *mutex)
{
    int state = load(&mutex->state, relaxed);
    if (state & MUTEX_LOCKED)
        return false;

    state = fetch_or(&mutex->state, MUTEX_LOCKED, relaxed);
    if (state & MUTEX_LOCKED)
        return false;

    thread_fence(&mutex->state, acquire);
    return true;
}

// 嘗試呼叫上鎖，在 128 內成功的話返回，不成功的話，嘗試將 state 變成 locked | sleeping，並不斷嘗試直到 state 直到變 locked，接著設定 thread_fence 去防止指令重新排列
static inline void mutex_lock(mutex_t *mutex)
{
#define MUTEX_SPINS 128
    for (int i = 0; i < MUTEX_SPINS; ++i) {
        if (mutex_trylock(mutex))
            return;
        spin_hint();
    }

    int state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);

    while (state & MUTEX_LOCKED) {
        futex_wait(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING);
        state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);
    }

    thread_fence(&mutex->state, acquire);
}

// 將狀態改成 0 ，假如還是 sleeping 狀態，呼叫 wake
static inline void mutex_unlock(mutex_t *mutex)
{
    int state = exchange(&mutex->state, 0, release);
    if (state & MUTEX_SLEEPING)
        // TODO: FFFF(&mutex->state, 1)
        futex_wake(&mutex->state, 1);
}

#endif