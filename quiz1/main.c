#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cond.h"
#include "futex.h"
#include "mutex.h"

struct clock {
    mutex_t mutex;
    cond_t cond;
    int ticks;
};

// 初始化 mutex 和 cond ，並把 ticks 設為 0
static void clock_init(struct clock *clock)
{
    mutex_init(&clock->mutex);
    cond_init(&clock->cond);
    clock->ticks = 0;
}

// 假設當 clock 的 ticks >= 0 並且 clock 的 ticks < 指定的 ticks 時，等待
// cond_wait 訊號，收到後如果達成前述條件則繼續等待 等跳出迴圈後，假如 clock 的
// ticks >= 指定的 ticks 回傳 true; 反之 false
static bool clock_wait(struct clock *clock, int ticks)
{
    mutex_lock(&clock->mutex);
    while (clock->ticks >= 0 && clock->ticks < ticks)
        cond_wait(&clock->cond, &clock->mutex);
    bool ret = clock->ticks >= ticks;
    mutex_unlock(&clock->mutex);
    return ret;
}

// ticks + 1 並通知所有的 cond
static void clock_tick(struct clock *clock)
{
    mutex_lock(&clock->mutex);
    if (clock->ticks >= 0)
        ++clock->ticks;
    mutex_unlock(&clock->mutex);
    cond_broadcast(&clock->cond, &clock->mutex);
}

// ticks - 1 並通知所有的 cond
static void clock_stop(struct clock *clock)
{
    mutex_lock(&clock->mutex);
    clock->ticks = -1;
    mutex_unlock(&clock->mutex);
    cond_broadcast(&clock->cond, &clock->mutex);
}

/* A node in a computation graph */
struct node {
    int index;
    struct clock *clock;
    struct node *parent;
    mutex_t mutex;
    cond_t cond;
    bool ready;
};

// 初始化節點，將主要的 clock, parent 放入，並且初始化 mutx, cond，以及將 ready
// 設為 false
static void node_init(int index,
                      struct clock *clock,
                      struct node *parent,
                      struct node *node)
{
    node->index = index;
    node->clock = clock;
    node->parent = parent;
    mutex_init(&node->mutex);
    cond_init(&node->cond);
    node->ready = false;
}

// 等待 cond 收到 signal 並且 ready 為 true
static void node_wait(struct node *node)
{
    mutex_lock(&node->mutex);
    while (!node->ready)
        cond_wait(&node->cond, &node->mutex);
    node->ready = false;
    mutex_unlock(&node->mutex);
}

// 將 ready 轉為 true，且送出 signal 訊號
static void node_signal(struct node *node)
{
    mutex_lock(&node->mutex);
    node->ready = true;
    mutex_unlock(&node->mutex);
    cond_signal(&node->cond, &node->mutex);
}

static void *thread_func(void *ptr)
{
    struct node *self = ptr;
    bool bit = false;
    printf("thread func: %d start ----\n", self->index);
    for (int i = 1; clock_wait(self->clock, i); ++i) {
        printf("thread func: %d -> %d ----\n", self->index, i);
        // 當有父節點在時，等待他接受到訊號
        if (self->parent)
            node_wait(self->parent);

        // 交替送 singal 或是執行時鐘
        if (bit) {
            node_signal(self);
        } else {
            clock_tick(self->clock);
        }
        bit = !bit;
    }

    node_signal(self);
    return NULL;
}

int main(void)
{
    struct clock clock;
    clock_init(&clock);

#define N_NODES 16
    // 建立 16 個節點的 linked list，並且所有的 node 都共用同一個 clock
    struct node nodes[N_NODES];
    node_init(0, &clock, NULL, &nodes[0]);
    for (int i = 1; i < N_NODES; ++i)
        node_init(i, &clock, &nodes[i - 1], &nodes[i]);

    // 依據節點數量產生相應數量的 thread 去執行 thread_func
    pthread_t threads[N_NODES];
    for (int i = 0; i < N_NODES; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, &nodes[i]) != 0)
            return EXIT_FAILURE;
    }

    // sleep(5);
    puts("Ready clock tick first");
    // 啟動第一次時鐘，啟動後 thread_func 中的 wait 收到後，就會開始執行下一步
    clock_tick(&clock);
    // sleep(1);
    printf("Until %d times\n", 1 << N_NODES);
    // 執行等待，等到 1 << N_NODES 秒後結束
    clock_wait(&clock, 1 << N_NODES);
    puts("Ready to stop clock");
    // 停止時鐘
    clock_stop(&clock);
    puts("clock stop");

    for (int i = 0; i < N_NODES; ++i) {
        if (pthread_join(threads[i], NULL) != 0)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}