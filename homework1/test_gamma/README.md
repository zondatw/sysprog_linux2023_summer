# Test Gamma

## Build

`gcc -Wall -o qsort qsort_mt.c -lpthread`


## Questions

```
延伸問題:

解釋上述程式碼運作原理
以 Thread Sanitizer 找出上述程式碼的 data race 並著手修正
研讀 專題: lib/sort.c，提出上述程式碼效能改進之規劃並予以實作
```


### Questions 2: (以 Thread Sanitizer 找出上述程式碼的 data race 並著手修正)

ref: [ThreadSanitizerCppManual](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)


test build: `clang qsort_mt.c -fsanitize=thread -fPIE -pie -g`


```
==================
WARNING: ThreadSanitizer: data race (pid=26478)
  Write of size 4 at 0x7b4000000080 by thread T1 (mutexes: write M0):
    #0 allocate_thread /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:212:27 (a.out+0xead6d) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_algo /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:315:16 (a.out+0xea687) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 qsort_thread /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:353:5 (a.out+0xe8b95) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Previous read of size 4 at 0x7b4000000080 by thread T2 (mutexes: write M1):
    #0 qsort_thread /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:344:16 (a.out+0xe8a0d) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Location is heap block of size 256 at 0x7b4000000000 allocated by main thread:
    #0 calloc <null> (a.out+0x75c3b) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_mt /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:133:19 (a.out+0xe7f26) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 main /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:513:13 (a.out+0xe97df) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Mutex M0 (0x7ffd6cdedad0) created at:
    #0 pthread_mutex_init <null> (a.out+0x8e458) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_mt /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:131:9 (a.out+0xe7f07) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 main /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:513:13 (a.out+0xe97df) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Mutex M1 (0x7b40000000a8) created at:
    #0 pthread_mutex_init <null> (a.out+0x8e458) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_mt /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:137:13 (a.out+0xe7f82) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 main /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:513:13 (a.out+0xe97df) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Thread T1 (tid=26480, running) created by main thread at:
    #0 pthread_create <null> (a.out+0x687a6) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_mt /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:145:13 (a.out+0xe80a0) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 main /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:513:13 (a.out+0xe97df) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

  Thread T2 (tid=26482, running) created by main thread at:
    #0 pthread_create <null> (a.out+0x687a6) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #1 qsort_mt /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:145:13 (a.out+0xe80a0) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)
    #2 main /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:513:13 (a.out+0xe97df) (BuildId: 78d43557cad1517b173e9905d5d5c9d48af862b1)

SUMMARY: ThreadSanitizer: data race /home/zonda/repos/sysprog/linux2023_summer/homework1/test_gamma/qsort_mt.c:212:27 in allocate_thread
==================

```

看到有說是 212 行的 allocate_thread 有問題，發現是 lock 的位置錯了，要在變更值之前，因此將程式變成
```c

/* Allocate an idle thread from the pool, lock its mutex, change its state to
 * work, decrease the number of idle threads, and return a pointer to its data
 * area.
 * Return NULL, if no thread is available.
 */
static struct qsort *allocate_thread(struct common *c)
{
    verify(pthread_mutex_lock(&c->mtx_al));
    for (int i = 0; i < c->nthreads; i++)
        if (c->pool[i].st == ts_idle) {
            c->idlethreads--;
            verify(pthread_mutex_lock(&c->pool[i].mtx_st));
            c->pool[i].st = ts_work;
            verify(pthread_mutex_unlock(&c->mtx_al));
            return (&c->pool[i]);
        }
    verify(pthread_mutex_unlock(&c->mtx_al));
    return (NULL);
}
```
