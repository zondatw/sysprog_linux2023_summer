#include <bits/types/struct_sched_param.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cond.h"
#include "mutex.h"


#define ERROR_HANDLER(x)                                            \
    do {                                                            \
        int e;                                                      \
        if ((e = x) != 0) {                                         \
            fprintf(stderr, "%s(%d) %s\n", __FILE__, __LINE__, #x); \
            return e;                                               \
        }                                                           \
    } while (0)

typedef enum priority_enum {
    PRIORITY_LOW,
    PRIORITY_MID,
    PRIORITY_HIGH,
} Priority_enum;

atomic int share_resource = 0;

struct context {
    mutex_t mutex;
};

static void context_init(struct context *ctx)
{
    mutex_init(&ctx->mutex);
}

static void *thread_low_func(void *ptr)
{
    struct context *ctx = ptr;
    mutex_lock(&ctx->mutex);
    printf("Thread low func: ready to sleep\n");
    usleep(1000 * 1000);  // 1s
    int val = load(&share_resource, relaxed);
    printf("Thread low func: execution [%d]\n", val);
    mutex_unlock(&ctx->mutex);
    return NULL;
}

static void *thread_mid_func(void *ptr)
{
    struct context *ctx = ptr;
    mutex_lock(&ctx->mutex);
    printf("Thread mid func: ready to sleep\n");
    usleep(2000);  // 0.001s
    fetch_sub(&share_resource, 1, relaxed);
    int val = load(&share_resource, relaxed);
    printf("Thread mid func: execution [%d]\n", val);
    mutex_unlock(&ctx->mutex);
    return NULL;
}

static void *thread_high_func(void *ptr)
{
    struct context *ctx = ptr;
    mutex_lock(&ctx->mutex);
    printf("Thread high func: ready to sleep\n");
    usleep(1000);  // 0.001s
    fetch_add(&share_resource, 1, relaxed);
    int val = load(&share_resource, relaxed);
    printf("Thread high func: execution [%d]\n", val);
    mutex_unlock(&ctx->mutex);
    return NULL;
}

static int get_priroity_num(Priority_enum priority)
{
    switch (priority) {
    case PRIORITY_LOW:
        return 10;
    case PRIORITY_MID:
        return 50;
    case PRIORITY_HIGH:
        return 90;
    };
    return -1;
}

static int pthread_create_priority(pthread_t *task_t,
                                   pthread_attr_t *attr,
                                   void *(*task_func)(void *),
                                   void *args,
                                   Priority_enum priority_level)
{
    struct sched_param params;
    int priority = get_priroity_num(priority_level);
    if (priority < 0) {
        return -1;
    }
    params.sched_priority = priority;
    ERROR_HANDLER(pthread_attr_setschedparam(attr, &params));
    ERROR_HANDLER(pthread_create(task_t, attr, task_func, args));
    return 0;
}

int main(void)
{
    // init thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    ERROR_HANDLER(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    ERROR_HANDLER(pthread_attr_setschedpolicy(&attr, SCHED_FIFO));

    // create thread with priority
    struct context ctx;
    context_init(&ctx);
    struct context ctx2;
    context_init(&ctx2);

    pthread_t low_t, mid_t, high_t;
    ERROR_HANDLER(pthread_create_priority(&low_t, &attr, thread_low_func, &ctx,
                                          PRIORITY_LOW));
    ERROR_HANDLER(pthread_create_priority(&mid_t, &attr, thread_mid_func, &ctx2,
                                          PRIORITY_MID));
    ERROR_HANDLER(pthread_create_priority(&high_t, &attr, thread_high_func,
                                          &ctx, PRIORITY_HIGH));

    // execute until finished
    ERROR_HANDLER(pthread_join(low_t, NULL));
    ERROR_HANDLER(pthread_join(mid_t, NULL));
    ERROR_HANDLER(pthread_join(high_t, NULL));

    return 0;
}