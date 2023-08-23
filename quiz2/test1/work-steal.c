/* A work-stealing scheduler is described in
 * Robert D. Blumofe, Christopher F. Joerg, Bradley C. Kuszmaul, Charles E.
 * Leiserson, Keith H. Randall, and Yuli Zhou. Cilk: An efficient multithreaded
 * runtime system. In Proceedings of the Fifth ACM SIGPLAN Symposium on
 * Principles and Practice of Parallel Programming (PPoPP), pages 207-216,
 * Santa Barbara, California, July 1995.
 * http://supertech.csail.mit.edu/papers/PPoPP95.pdf
 *
 * However, that refers to an outdated model of Cilk; an update appears in
 * the essential idea of work stealing mentioned in Leiserson and Platt,
 * Programming Parallel Applications in Cilk
 */

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct work_internal;

/* A 'task_t' represents a function pointer that accepts a pointer to a 'work_t'
 * struct as input and returns another 'work_t' struct as output. The input to
 * this function is always a pointer to the encompassing 'work_t' struct.
 *
 * It is worth considering whether to include information about the executing
 * thread's identifier when invoking the task. This information might be
 * beneficial for supporting thread-local accumulators in cases of commutative
 * reductions. Additionally, it could be useful to determine the destination
 * worker's queue for appending further tasks.
 *
 * The 'task_t' trampoline is responsible for delivering the subsequent unit of
 * work to be executed. It returns the next work item if it is prepared for
 * execution, or NULL if the task is not ready to proceed.
 */
typedef struct work_internal *(*task_t)(struct work_internal *);

typedef struct work_internal {
    task_t code;
    atomic_int join_count;
    void *args[];
} work_t;

/* These are non-NULL pointers that will result in page faults under normal
 * circumstances, used to verify that nobody uses non-initialized entries.
 */
static work_t *EMPTY = (work_t *) 0x100, *ABORT = (work_t *) 0x200;

/* work_t-stealing deque */

typedef struct {
    atomic_size_t size;
    _Atomic work_t *buffer[];
} array_t;

typedef struct {
    /* Assume that they never overflow */
    atomic_size_t top, bottom;
    _Atomic(array_t *) array;
} deque_t;


// 初始化 deque ， top 和 bottom 指標先指到 0，array 大小為 sizeof(array_t) +
// sizeof(work_t *) * size_hint
void init(deque_t *q, int size_hint)
{
    atomic_init(&q->top, 0);
    atomic_init(&q->bottom, 0);
    array_t *a = malloc(sizeof(array_t) + sizeof(work_t *) * size_hint);
    atomic_init(&a->size, size_hint);
    atomic_init(&q->array, a);
}

// 分配新的兩倍大小的 array ，並把資料都依據原本的 index 搬過去
void resize(deque_t *q)
{
    array_t *a = atomic_load_explicit(&q->array, memory_order_relaxed);
    size_t old_size = a->size;
    size_t new_size = old_size * 2;
    array_t *new = malloc(sizeof(array_t) + sizeof(work_t *) * new_size);
    atomic_init(&new->size, new_size);
    size_t t = atomic_load_explicit(&q->top, memory_order_relaxed);
    size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed);
    for (size_t i = t; i < b; i++)
        new->buffer[i % new_size] = a->buffer[i % old_size];

    atomic_store_explicit(&q->array, new, memory_order_relaxed);
    /* The question arises as to the appropriate timing for releasing memory
     * associated with the previous array denoted by *a. In the original Chase
     * and Lev paper, this task was undertaken by the garbage collector, which
     * presumably possessed knowledge about ongoing steal operations by other
     * threads that might attempt to access data within the array.
     *
     * In our context, the responsible deallocation of *a cannot occur at this
     * point, as another thread could potentially be in the process of reading
     * from it. Thus, we opt to abstain from freeing *a in this context,
     * resulting in memory leakage. It is worth noting that our expansion
     * strategy for these queues involves consistent doubling of their size;
     * this design choice ensures that any leaked memory remains bounded by the
     * memory actively employed by the functional queues.
     */
}

// 當 top <= bottom - 1 時，從 bottom - 1 index 取得 work，當 top == bottom - 1
// 時，將 top + 1 ，原因是要讓新的 work 放在空的位置上，接著將 bottom -1 再 +1
// 儲存起來; 如果 top > bottom - 1 時，回傳 empty 並把 bottom - 1 + 1 儲存起來
work_t *take(deque_t *q)
{
    size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed) - 1;
    array_t *a = atomic_load_explicit(&q->array, memory_order_relaxed);
    atomic_store_explicit(&q->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    size_t t = atomic_load_explicit(&q->top, memory_order_relaxed);
    work_t *x;
    if (t <= b) {
        /* Non-empty queue */
        x = atomic_load_explicit(&a->buffer[b % a->size], memory_order_relaxed);
        if (t == b) {
            /* Single last element in queue */
            /* TODO: if (!atomic_compare_exchange_strong_explicit(&q->top, &t,
               AAAA, memory_order_seq_cst, memory_order_relaxed))*/
            if (!atomic_compare_exchange_strong_explicit(&q->top, &t, t + 1,
                                                         memory_order_seq_cst,
                                                         memory_order_relaxed))
                /* Failed race */
                x = EMPTY;
            // TODO: atomic_store_explicit(&q->bottom, BBBB,
            // memory_order_relaxed);
            atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
        }
    } else { /* Empty queue */
        x = EMPTY;
        // TODO: atomic_store_explicit(&q->bottom, CCCC, memory_order_relaxed);
        atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
    }
    return x;
}

// 新增 work 到 queue 中，並將 bottom + 1
void push(deque_t *q, work_t *w)
{
    size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed);
    size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
    array_t *a = atomic_load_explicit(&q->array, memory_order_relaxed);
    if (b - t > a->size - 1) { /* Full queue */
        resize(q);
        a = atomic_load_explicit(&q->array, memory_order_relaxed);
    }
    atomic_store_explicit(&a->buffer[b % a->size], w, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    // TODO: atomic_store_explicit(&q->bottom, DDDD, memory_order_relaxed);
    atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
}

// 從 queue 的 top index 取走 work，並將 top + 1
work_t *steal(deque_t *q)
{
    size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    size_t b = atomic_load_explicit(&q->bottom, memory_order_acquire);
    work_t *x = EMPTY;
    if (t < b) {
        /* Non-empty queue */
        array_t *a = atomic_load_explicit(&q->array, memory_order_consume);
        x = atomic_load_explicit(&a->buffer[t % a->size], memory_order_relaxed);
        /* TODO: if (!atomic_compare_exchange_strong_explicit(
                &q->top, &t, EEEE, memory_order_seq_cst,
           memory_order_relaxed))*/
        if (!atomic_compare_exchange_strong_explicit(
                &q->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed))
            /* Failed race */
            return ABORT;
    }
    return x;
}

#define N_THREADS 24
deque_t *thread_queues;

atomic_bool done;

/* Returns the subsequent item available for processing, or NULL if no items
 * are remaining.
 */
static work_t *do_one_work(int id, work_t *work)
{
    printf("work item %d running item %p\n", id, work);
    return (*(work->code))(work);
}

void do_work(int id, work_t *work)
{
    while (work)
        work = do_one_work(id, work);
}

/* Returns the next item to be processed, or NULL if there are no remaining
 * items.
 */
work_t *join_work(work_t *work)
{
    int old_join_count = atomic_fetch_sub(&work->join_count, 1);
    if (old_join_count == 1)
        return work;
    return NULL;
}

// 從自己的 queue 中取得 work 並執行，如果沒有 work ，嘗試從別人的 queue 取得
// work，如果也沒有 work 了，則將 done 變為 true，並結束 thread
void *thread(void *payload)
{
    int id = *(int *) payload;
    deque_t *my_queue = &thread_queues[id];
    while (true) {
        work_t *work = take(my_queue);
        if (work != EMPTY) {
            do_work(id, work);
        } else {
            /* Currently, there is no work present in my own queue */
            work_t *stolen = EMPTY;
            for (int i = 0; i < N_THREADS; ++i) {
                if (i == id)
                    continue;
                stolen = steal(&thread_queues[i]);
                if (stolen == ABORT) {
                    i--;
                    continue; /* Try again at the same i */
                } else if (stolen == EMPTY)
                    continue;

                /* Found some work to do */
                break;
            }
            if (stolen == EMPTY) {
                /* Despite the previous observation of all queues being devoid
                 * of tasks during the last examination, there exists
                 * a possibility that additional work items have been introduced
                 * subsequently. To account for this scenario, a state of active
                 * waiting is adopted, wherein the program continues to loop
                 * until the global "done" flag becomes set, indicative of
                 * potential new work additions.
                 */
                if (atomic_load(&done))
                    break;
                continue;
            } else {
                do_work(id, stolen);
            }
        }
    }
    printf("work item %d finished\n", id);
    return NULL;
}

// 顯示 item 的 payload
work_t *print_task(work_t *w)
{
    int *payload = (int *) w->args[0];
    int item = *payload;
    printf("Did item %p with payload %d\n", w, item);
    work_t *cont = (work_t *) w->args[1];
    free(payload);
    free(w);
    return join_work(cont);
}

// 將 done 變成 true
work_t *done_task(work_t *w)
{
    free(w);
    atomic_store(&done, true);
    return NULL;
}

int main(int argc, char **argv)
{
    /* Check that top and bottom are 64-bit so they never overflow */
    static_assert(sizeof(atomic_size_t) == 8,
                  "Assume atomic_size_t is 8 byte wide");

    pthread_t threads[N_THREADS];
    int tids[N_THREADS];
    thread_queues = malloc(N_THREADS * sizeof(deque_t));
    int nprints = 10;

    atomic_store(&done, false);
    work_t *done_work = malloc(sizeof(work_t));
    done_work->code = &done_task;
    done_work->join_count = N_THREADS * nprints;

    // 建立 N_THREADS 個 thread queue
    for (int i = 0; i < N_THREADS; ++i) {
        tids[i] = i;
        init(&thread_queues[i], 8);
        // 每個 thread queue 擁有 nprints 個 work
        for (int j = 0; j < nprints; ++j) {
            work_t *work = malloc(sizeof(work_t) + 2 * sizeof(int *));
            work->code = &print_task;
            work->join_count = 0;
            int *payload = malloc(sizeof(int));
            *payload = 1000 * i + j;

            // 空間來自 2 * sizeof(int *)
            work->args[0] = payload;
            work->args[1] = done_work;
            push(&thread_queues[i], work);
        }
    }

    // 執行 N_THREADS 個 thread
    for (int i = 0; i < N_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread, &tids[i]) != 0) {
            perror("Failed to start the thread");
            exit(EXIT_FAILURE);
        }
    }

    // 等待 N_THREADS 個 thread 執行完畢
    for (int i = 0; i < N_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join the thread");
            exit(EXIT_FAILURE);
        }
    }
    printf("Expect %d lines of output (including this one)\n",
           2 * N_THREADS * nprints + N_THREADS + 2);

    return 0;
}