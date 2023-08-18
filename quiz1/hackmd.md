# 2023 Homework2 (quiz1)

contributed by < [zondatw](https://github.com/zondatw/sysprog_linux2023_summer) >

## 測驗 1 - 1 
> 解釋上述程式碼運作原理，應該涵蓋測試方法、futex 使用方式、Linux 核心開發者針對 POSIX Threads 強化哪些相關 futex 實作機制等等


### 主程式

主要的有 clock 負責用來啟動。

clock 的 struct 為：
```c
struct clock {
    mutex_t mutex;
    cond_t cond;
    int ticks;
};
```

附帶的功能有：

1. clock_init: 初始化 mutex 和 cond ，並把 ticks 設為 0
2. clock_wait: 假設當 clock 的 ticks >= 0 並且 clock 的 ticks < 指定的 ticks 時，等待 cond_wait 訊號，收到後如果達成前述條件則繼續等待; 等跳出迴圈後，假如 clock 的 ticks >= 指定的 ticks 回傳 true; 反之 false
3. clock_tick: clock 的 ticks + 1 並通知所有的 cond
4. clock_stop: clock 的 ticks - 1 並通知所有的 cond

接著有個 node 負責用來執行事項。

node 的 struct 為：
```c
struct node {
    struct clock *clock;
    struct node *parent;
    mutex_t mutex;
    cond_t cond;
    bool ready;
};
```

附帶的功能有：

1. node_init: 初始化節點，將主要的 clock 和 parent node 放入，並且初始化 mutx, cond，以及將 ready 設為 false
2. node_wait: 等待 cond 收到 signal 並且 ready 為 true
3. node_signal: 將 ready 轉為 true，且送出 signal 訊號

接著 thread_func 主要功能為不斷的跑迴圈，每次的 index 會用來 clock_wait 的指定 tick 使用，當 clock 的 tick 計數到指定的 tick 時，執行迴圈中的事項

```c
for (int i = 1; clock_wait(self->clock, i); ++i) {
    ...
}
```

迴圈中的功能的流程：
1. 當父節點存在時，等待接受父節點訊號
2. 接著會交替送 signal 或是 clock 計數 +1

解釋完上述的功能後，主流程為：
1. 初始化 clock
2. 建立 N 個 node 的 linked list，每個 node 除了第一個之外，會去連到上一個 node： Null <- node.parent <- node.parent <- node.parent ... 以及在這邊每個 node 的 clock 都是共用前面產生的
3. 依據 node 數量產生相應數量的 thread 去執行 thread_func
4. 啟動第一次時鐘，啟動後 thread_func 中的 wait 收到後，就會開始執行下一步
5. 執行等待，等到 1 << N_NODES 秒後結束
7. 停止時鐘
8. 等待 thread 都執行完畢

解釋 `1 << N_NODES`，這裡是 thread 0 的迴圈執行的最大次數，而 thread 1 因為 thread_func 的行為是等待父節點發通知後，才會輪替的執行送 signal 或 clock + 1 ，因此為 thread 1 的次數 / 2 以此類推，但是當 N_NODES 為奇數時，除了 thread 0 以外會多 +1

修改 code 去做驗證

```c
struct node {
    int index;
    struct clock *clock;
    struct node *parent;
    mutex_t mutex;
    cond_t cond;
    bool ready;
};

static void *thread_func(void *ptr)
{
    struct node *self = ptr;
    bool bit = false;
    printf("thread func: %d start \n", self->index);
    for (int i = 1; clock_wait(self->clock, i); ++i) {
        printf("thread func: %d -> %d \n", self->index, i);
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
    ... 
    node_init(0, &clock, NULL, &nodes[0]);
    for (int i = 1; i < N_NODES; ++i)
        node_init(i, &clock, &nodes[i - 1], &nodes[i]);
    ...
}

```

當 N_NODES 為 2 時：
thread 0 的迴圈執行 4 次，thread 1 執行 2 次
```
thread func: 0 -> 1 ----
thread func: 0 -> 2 ----
thread func: 1 -> 1 ----
thread func: 1 -> 2 ----
thread func: 0 -> 3 ----
thread func: 0 -> 4 ----
```

當 N_NODES 為 3 時：
thread 0 的迴圈執行 8 次，thread 1 執行 5 次，thread 2 執行 3 次
```
thread func: 1 -> 1 ----
thread func: 0 -> 1 ----
thread func: 0 -> 2 ----
thread func: 1 -> 2 ----
thread func: 0 -> 3 ----
thread func: 0 -> 4 ----
thread func: 1 -> 3 ----
thread func: 2 -> 1 ----
thread func: 2 -> 2 ----
thread func: 0 -> 5 ----
thread func: 0 -> 6 ----
thread func: 1 -> 4 ----
thread func: 0 -> 7 ----
thread func: 0 -> 8 ----
thread func: 1 -> 5 ----
thread func: 2 -> 3 ----
```

以及測試執行時間上，直接使用 time 去看，在 N_NODES 為 16，linux （futex）版本是比較快的：
```shell
Running test_pthread ...
real    0m0.881s
user    0m0.803s
sys     0m0.561s
[OK]
Running test_linux ...
real    0m0.520s
user    0m1.264s
sys     0m0.216s
[OK]
```

#### cond

struct 為：
```c
typedef struct {
    atomic int seq;
} cond_t;
```

* cond_wait: 等待 cond 的訊號。收到後返回
* cond_signal: 將 seq + 1，並呼叫 wake
* cond_broadcast: 將 seq + 1，並呼叫 requeue


### futex

這邊是用 system call 的方式去呼叫使用

```c
/* Atomically check if '*futex == value', and if so, go to sleep */
static inline void futex_wait(atomic int *futex, int value)
{
    syscall(SYS_futex, futex, FUTEX_WAIT_PRIVATE, value, NULL);
}

/* Wake up 'limit' threads currently waiting on 'futex' */
static inline void futex_wake(atomic int *futex, int limit)
{
    syscall(SYS_futex, futex, FUTEX_WAKE_PRIVATE, limit);
}

/* Wake up 'limit' waiters, and re-queue the rest onto a different futex */
static inline void futex_requeue(atomic int *futex,
                                 int limit,
                                 atomic int *other)
{
    syscall(SYS_futex, futex, FUTEX_REQUEUE_PRIVATE, limit, INT_MAX, other);
}
```

* futex_wait: 等待期望值 value ，否則繼續睡
* futex_wake: 最大喚醒 limit 個等待者
* futex_requeue: 最大喚醒 limit 等待者，最大剩餘 INT_MAX 個等待者放到 other 中 

從 [futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html) 中有看到：

syscall 格式：
```c
#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>

long syscall(SYS_futex, uint32_t *uaddr, int futex_op, uint32_t val,
            const struct timespec *timeout,   /* or: uint32_t val2 */
            uint32_t *uaddr2, uint32_t val3);
```

```
       FUTEX_WAIT (since Linux 2.6.0)
              This operation tests that the value at the futex word
              pointed to by the address uaddr still contains the
              expected value val, and if so, then sleeps waiting for a
              FUTEX_WAKE operation on the futex word.  The load of the
              value of the futex word is an atomic memory access (i.e.,
              using atomic machine instructions of the respective
              architecture).  This load, the comparison with the
              expected value, and starting to sleep are performed
              atomically and totally ordered with respect to other futex
              operations on the same futex word.  If the thread starts
              to sleep, it is considered a waiter on this futex word.
              If the futex value does not match val, then the call fails
              immediately with the error EAGAIN.

              The purpose of the comparison with the expected value is
              to prevent lost wake-ups.  If another thread changed the
              value of the futex word after the calling thread decided
              to block based on the prior value, and if the other thread
              executed a FUTEX_WAKE operation (or similar wake-up) after
              the value change and before this FUTEX_WAIT operation,
              then the calling thread will observe the value change and
              will not start to sleep.

              If the timeout is not NULL, the structure it points to
              specifies a timeout for the wait.  (This interval will be
              rounded up to the system clock granularity, and is
              guaranteed not to expire early.)  The timeout is by
              default measured according to the CLOCK_MONOTONIC clock,
              but, since Linux 4.5, the CLOCK_REALTIME clock can be
              selected by specifying FUTEX_CLOCK_REALTIME in futex_op.
              If timeout is NULL, the call blocks indefinitely.

              Note: for FUTEX_WAIT, timeout is interpreted as a relative
              value.  This differs from other futex operations, where
              timeout is interpreted as an absolute value.  To obtain
              the equivalent of FUTEX_WAIT with an absolute timeout,
              employ FUTEX_WAIT_BITSET with val3 specified as
              FUTEX_BITSET_MATCH_ANY.

              The arguments uaddr2 and val3 are ignored.

       FUTEX_WAKE (since Linux 2.6.0)
              This operation wakes at most val of the waiters that are
              waiting (e.g., inside FUTEX_WAIT) on the futex word at the
              address uaddr.  Most commonly, val is specified as either
              1 (wake up a single waiter) or INT_MAX (wake up all
              waiters).  No guarantee is provided about which waiters
              are awoken (e.g., a waiter with a higher scheduling
              priority is not guaranteed to be awoken in preference to a
              waiter with a lower priority).

              The arguments timeout, uaddr2, and val3 are ignored.
              
       FUTEX_REQUEUE (since Linux 2.6.0)
              This operation performs the same task as FUTEX_CMP_REQUEUE
              (see below), except that no check is made using the value
              in val3.  (The argument val3 is ignored.)
              
       FUTEX_CMP_REQUEUE (since Linux 2.6.7)
              This operation first checks whether the location uaddr
              still contains the value val3.  If not, the operation
              fails with the error EAGAIN.  Otherwise, the operation
              wakes up a maximum of val waiters that are waiting on the
              futex at uaddr.  If there are more than val waiters, then
              the remaining waiters are removed from the wait queue of
              the source futex at uaddr and added to the wait queue of
              the target futex at uaddr2.  The val2 argument specifies
              an upper limit on the number of waiters that are requeued
              to the futex at uaddr2.
              
              ...
```

說有 return value

       FUTEX_WAIT
              Returns 0 if the caller was woken up.  Note that a wake-up
              can also be caused by common futex usage patterns in
              unrelated code that happened to have previously used the
              futex word's memory location (e.g., typical futex-based
              implementations of Pthreads mutexes can cause this under
              some conditions).  Therefore, callers should always
              conservatively assume that a return value of 0 can mean a
              spurious wake-up, and use the futex word's value (i.e.,
              the user-space synchronization scheme) to decide whether
              to continue to block or not.

       FUTEX_WAKE
              Returns the number of waiters that were woken up.

       FUTEX_REQUEUE
              Returns the number of waiters that were woken up.

有分 PRIVATE 的 flag，而我們這次都使用 _PRIVATE，表示這個  futex 是 process 私有的，其他 process 不能使用

```
       FUTEX_PRIVATE_FLAG (since Linux 2.6.22)
              This option bit can be employed with all futex operations.
              It tells the kernel that the futex is process-private and
              not shared with another process (i.e., it is being used
              for synchronization only between threads of the same
              process).  This allows the kernel to make some additional
              performance optimizations.

              As a convenience, <linux/futex.h> defines a set of
              constants with the suffix _PRIVATE that are equivalents of
              all of the operations listed below, but with the
              FUTEX_PRIVATE_FLAG ORed into the constant value.  Thus,
              there are FUTEX_WAIT_PRIVATE, FUTEX_WAKE_PRIVATE, and so
              on.
```




### atomic

關於 atomic 操作的 marco，方便使用時可以設定 memory_order

example:
```c
// define
#define load(obj, order) atomic_load_explicit(obj, memory_order_##order)

// use
int seq = load(&cond->seq, relaxed);
```

### mutex

struct 為:
```c
typedef struct {
    atomic int state;
} mutex_t;
```

* mutex_trylock: 嘗試上鎖，首先會取得現在的狀態，如果是上鎖狀態返回 false，接著嘗試上鎖後，判斷狀態，如果不是上鎖，返回 false，接著設定 thread_fence 去防止指令重新排列
* mutex_lock: 嘗試呼叫上鎖，在 128 次內成功的話返回，不成功的話，嘗試將 state 變成 locked | sleeping，並不斷嘗試直到 state 直到變 locked，接著設定 thread_fence 去防止指令重新排列
* mutex_unlock: 將狀態改成 0 ，假如還是 sleeping 狀態，呼叫 wake

### spinlock

struct 為:
```c
typedef struct {
    atomic bool state;
} spinlock_t;
```

透過 atomic 去設定 state true 時為 lock，false 為 unlock，
在當要 lock 前，透過 atomic relaxed 去確認現在的 state 為 flase 後，再嘗試透過 atomic acquire 將 state 改為 true



## 測驗 1 - 2 

> 修改第 1 次作業的測驗 γ 提供的並行版本快速排序法實作，使其得以搭配上述 futex 程式碼運作

直接套原本的程式去改來用;

將原本的 pthread_mutex_t 和 pthread_cond_t，換成使用 mutex_t 和 cond_t

```diff
 /* Variant part passed to qsort invocations. */
 struct qsort {
-    enum thread_state st;   /* For coordinating work. */
-    struct common *common;  /* Common shared elements. */
-    void *a;                /* Array base. */
-    size_t n;               /* Number of elements. */
-    pthread_t id;           /* Thread id. */
-    pthread_mutex_t mtx_st; /* For signalling state change. */
-    pthread_cond_t cond_st; /* For signalling state change. */
+    enum thread_state st;  /* For coordinating work. */
+    struct common *common; /* Common shared elements. */
+    void *a;               /* Array base. */
+    size_t n;              /* Number of elements. */
+    pthread_t id;          /* Thread id. */
+    // pthread_mutex_t mtx_st; /* For signalling state change. */
+    // pthread_cond_t cond_st; /* For signalling state change. */
+    mutex_t mtx_st;
+    cond_t cond_st;
 };
 
  struct common {
-    int swaptype;           /* Code to use for swapping */
-    size_t es;              /* Element size. */
-    void *thunk;            /* Thunk for qsort_r */
-    cmp_t *cmp;             /* Comparison function */
-    int nthreads;           /* Total number of pool threads. */
-    int idlethreads;        /* Number of idle threads in pool. */
-    int forkelem;           /* Minimum number of elements for a new thread. */
-    struct qsort *pool;     /* Fixed pool of threads. */
-    pthread_mutex_t mtx_al; /* For allocating threads in the pool. */
+    int swaptype;       /* Code to use for swapping */
+    size_t es;          /* Element size. */
+    void *thunk;        /* Thunk for qsort_r */
+    cmp_t *cmp;         /* Comparison function */
+    int nthreads;       /* Total number of pool threads. */
+    int idlethreads;    /* Number of idle threads in pool. */
+    int forkelem;       /* Minimum number of elements for a new thread. */
+    struct qsort *pool; /* Fixed pool of threads. */
+    // pthread_mutex_t mtx_al; /* For allocating threads in the pool. */
+    mutex_t mtx_al;
 };
```

修改 init 相關程式:

```diff
@@ -127,23 +140,16 @@ void qsort_mt(void *a,
         goto f1;
     errno = 0;
     /* Try to initialize the resources we need. */
-    if (pthread_mutex_init(&c.mtx_al, NULL) != 0)
-        goto f1;
+    mutex_init(&c.mtx_al);
     if ((c.pool = calloc(maxthreads, sizeof(struct qsort))) == NULL)
         goto f2;
     for (islot = 0; islot < maxthreads; islot++) {
         qs = &c.pool[islot];
-        if (pthread_mutex_init(&qs->mtx_st, NULL) != 0)
-            goto f3;
-        if (pthread_cond_init(&qs->cond_st, NULL) != 0) {
-            verify(pthread_mutex_destroy(&qs->mtx_st));
-            goto f3;
-        }
+        mutex_init(&qs->mtx_st);
+        cond_init(&qs->cond_st);
         qs->st = ts_idle;
         qs->common = &c;
         if (pthread_create(&qs->id, NULL, qsort_thread, qs) != 0) {
-            verify(pthread_mutex_destroy(&qs->mtx_st));
-            verify(pthread_cond_destroy(&qs->cond_st));
             goto f3;
         }
     }
```

將 lock 和 signal 等功能都換掉

```diff
@@ -163,31 +169,28 @@ void qsort_mt(void *a,

     /* Hand out the first work batch. */
     qs = &c.pool[0];
-    verify(pthread_mutex_lock(&qs->mtx_st));
+    verify(mutex_lock(&qs->mtx_st));
     qs->a = a;
     qs->n = n;
     qs->st = ts_work;
     c.idlethreads--;
-    verify(pthread_cond_signal(&qs->cond_st));
-    verify(pthread_mutex_unlock(&qs->mtx_st));
+    verify(cond_signal(&qs->cond_st, &qs->mtx_st));
+    verify(mutex_unlock(&qs->mtx_st));

     /* Wait for all threads to finish, and free acquired resources. */
 f3:
     for (i = 0; i < islot; i++) {
         qs = &c.pool[i];
         if (bailout) {
-            verify(pthread_mutex_lock(&qs->mtx_st));
+            verify(mutex_lock(&qs->mtx_st));
             qs->st = ts_term;
-            verify(pthread_cond_signal(&qs->cond_st));
-            verify(pthread_mutex_unlock(&qs->mtx_st));
+            verify(cond_signal(&qs->cond_st, &qs->mtx_st));
+            verify(mutex_unlock(&qs->mtx_st));
         }
-        verify(pthread_join(qs->id, NULL));
-        verify(pthread_mutex_destroy(&qs->mtx_st));
-        verify(pthread_cond_destroy(&qs->cond_st));
+        verifyOrig(pthread_join(qs->id, NULL));
     }
     free(c.pool);
 f2:
-    verify(pthread_mutex_destroy(&c.mtx_al));
     if (bailout) {
         fprintf(stderr, "Resource initialization failed; bailing out.\n");
     f1:
@@ -204,16 +207,16 @@ f2:
  */
 static struct qsort *allocate_thread(struct common *c)
 {
-    verify(pthread_mutex_lock(&c->mtx_al));
+    verify(mutex_lock(&c->mtx_al));
     for (int i = 0; i < c->nthreads; i++)
         if (c->pool[i].st == ts_idle) {
             c->idlethreads--;
+            verify(mutex_lock(&c->pool[i].mtx_st));
             c->pool[i].st = ts_work;
-            verify(pthread_mutex_lock(&c->pool[i].mtx_st));
-            verify(pthread_mutex_unlock(&c->mtx_al));
+            verify(mutex_unlock(&c->mtx_al));
             return (&c->pool[i]);
         }
-    verify(pthread_mutex_unlock(&c->mtx_al));
+    verify(mutex_unlock(&c->mtx_al));
     return (NULL);
 }

@@ -314,8 +317,8 @@ nevermind:
         (qs2 = allocate_thread(c)) != NULL) {
         qs2->a = a;
         qs2->n = nl;
-        verify(pthread_cond_signal(&qs2->cond_st));
-        verify(pthread_mutex_unlock(&qs2->mtx_st));
+        verify(cond_signal(&qs2->cond_st, &qs2->mtx_st));
+        verify(mutex_unlock(&qs2->mtx_st));
     } else if (nl > 0) {
         qs->a = a;
         qs->n = nl;
@@ -339,10 +342,10 @@ static void *qsort_thread(void *p)
     c = qs->common;
 again:
     /* Wait for work to be allocated. */
-    verify(pthread_mutex_lock(&qs->mtx_st));
+    verify(mutex_lock(&qs->mtx_st));
     while (qs->st == ts_idle)
-        verify(pthread_cond_wait(&qs->cond_st, &qs->mtx_st));
-    verify(pthread_mutex_unlock(&qs->mtx_st));
+        verify(cond_wait(&qs->cond_st, &qs->mtx_st));
+    verify(mutex_unlock(&qs->mtx_st));
     if (qs->st == ts_term) {
         return NULL;
     }
@@ -350,7 +353,7 @@ again:

     qsort_algo(qs);

-    verify(pthread_mutex_lock(&c->mtx_al));
+    verify(mutex_lock(&c->mtx_al));
     qs->st = ts_idle;
     c->idlethreads++;
     if (c->idlethreads == c->nthreads) {
@@ -358,15 +361,15 @@ again:
             qs2 = &c->pool[i];
             if (qs2 == qs)
                 continue;
-            verify(pthread_mutex_lock(&qs2->mtx_st));
+            verify(mutex_lock(&qs2->mtx_st));
             qs2->st = ts_term;
-            verify(pthread_cond_signal(&qs2->cond_st));
+        verify(cond_wait(&qs->cond_st, &qs->mtx_st));
+    verify(mutex_unlock(&qs->mtx_st));
     if (qs->st == ts_term) {
         return NULL;
     }
@@ -350,7 +353,7 @@ again:

     qsort_algo(qs);

-    verify(pthread_mutex_lock(&c->mtx_al));
+    verify(mutex_lock(&c->mtx_al));
     qs->st = ts_idle;
     c->idlethreads++;
     if (c->idlethreads == c->nthreads) {
@@ -358,15 +361,15 @@ again:
             qs2 = &c->pool[i];
             if (qs2 == qs)
                 continue;
-            verify(pthread_mutex_lock(&qs2->mtx_st));
+            verify(mutex_lock(&qs2->mtx_st));
             qs2->st = ts_term;
-            verify(pthread_cond_signal(&qs2->cond_st));
-            verify(pthread_mutex_unlock(&qs2->mtx_st));
+            verify(cond_signal(&qs2->cond_st, &qs2->mtx_st));
+            verify(mutex_unlock(&qs2->mtx_st));
         }
-        verify(pthread_mutex_unlock(&c->mtx_al));
+        verify(mutex_unlock(&c->mtx_al));
         return NULL;
     }
-    verify(pthread_mutex_unlock(&c->mtx_al));
+    verify(mutex_unlock(&c->mtx_al));
     goto again;
 }
```

build code

```just
original_source_code := "qsort_mt_orig.c"
source_code := "qsort_mt.c"
original_exe := "qsort"
exe := "qsort_futex"
cflags := "-Wall -g -O2 -D_GNU_SOURCE -fsanitize=thread -lpthread"

@build:
    gcc -o {{original_exe}} {{original_source_code}} {{cflags}}
    gcc -o {{exe}} {{source_code}} {{cflags}}
```

測試：

```
$ ./qsort -t
15.9 30.5 0.0897
$ ./qsort_futex -t
15.8 30.4 0.0732
```

測試加上 `time` 和 verify

```
$ time ./qsort -tv
15.9 30.4 0.0697

real    0m16.227s
user    0m30.748s
sys     0m0.077s

$ time ./qsort_futex -tv
15.9 30.4 0.0629

real    0m16.213s
user    0m30.740s
sys     0m0.070s
```

futex 版本有快一點點

## 測驗 1 - 3 
> 研讀〈並行程式設計: 建立相容於 POSIX Thread 的實作〉，在上述程式碼的基礎之上，實作 priority inheritance mutex 並確認與 glibc 實作行為相同，應有對應的 PI 測試程式碼


## 測驗 1 - 4 
> 比照 skinny-mutex，設計 POSIX Threads 風格的 API，並利用內附的 perf.c (斟酌修改) 確認執行模式符合預期，過程中也該比較 glibc 的 POSIX Threads 效能表現


## Reference

### Atomic

[Concurrency & Atomic 學習筆記](https://hackmd.io/@meyr543/rkUutrWQF#Explicit-function)
[C11标准库中的atomic原子操作](http://ericnode.info/post/atomic_in_c11/)
[vs的atomic和linux的stdatomic.h的原子操作的基本用法](https://blog.csdn.net/dong_beijing/article/details/104925408)

### Thread_fence

[C++ memory order循序渐进（四）—— 在std::atomic_thread_fence 上应用std::memory_order实现不同的内存序](https://blog.csdn.net/wxj1992/article/details/103917093)

### futex

[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)
