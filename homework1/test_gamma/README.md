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

### Questions 1: (解釋上述程式碼運作原理)

med3：
```
取 3 個數中的中間值
```

qsort_mt：
```
產生 n 個 thread 去執行 qsort_thead;
接著等待第一個 thread 執行完成後，會等待其他 thread 執行完成，當 thread 執行完成時釋放資源
```

qsort_thread：
```
當狀態為 ts_idle 時等待狀態送達，收到狀態變更後，當狀態為 ts_term 就結束 function，如果為 ts_work 則繼續執行程式，接著執行  qsort_algo 程式;
執行 qsort_algo 後，將狀態改成 ts_idle 並讓idlethreads 加一;
接著當 idlethread 和總 thread 數量不相等時，再次等待重複等待 ts_idle 的狀態變更;
如果相等，則將除了自己以外的 thread 狀態都變成 ts_term ，然後結束 function
```

qsort_algo:
```
a 為 array 的 pointer
n 為有幾個 element
當 n < 7 時， 用氣泡排序後結束 function;
接著取得中間值後，當 n > 7 時， 會去取得最左邊的值和最右邊的值，重新以這三個值，判斷中間的值為何，但如果 n > 40 時，會將用另一種方式重新取得中間值，這塊還不確定為什麼要這樣做。

接著將中間值和最左邊的值交換;
設定左往右找值的 index和最左邊基底 index，初始值為第二個位置;
設定右往左找值的 index和最右邊基底 index，初始值為最後一個位置;

執行交換位置的迴圈;
剛開始從左往右找值，找到大於 a 的值時跳出迴圈，或皆小於等於則跑到等於（右往左）的 index 時結束迴圈，掃描過程中，如果有掃到跟 a 相等的值時，往最左邊放，並把最左邊基底 index 加一;
接著從右往左找值，找到小於 a 的值時跳出迴圈，或皆大於等於則跑到等於（左往右）的 index 時結束迴圈，掃描過程中，如果有掃到跟 a 相等的值時，往最右放，並把最右邊基底 index 減一;

經過上面的查找後，如果 （左往右）的 index 大於 (右往左）的 index，結束迴圈，沒有的話就交換兩個 index 的值，並且將 (右往左）的 index 減一，（左往右）的 index 加一，繼續執行迴圈

接著以 （左往右）的 index 和 (右往左） index 的交會處，將 a 值該位置放，算是將基準中放到比較中間的位置，並且將上面查找時，發現與 a 值相等的值集中到基準旁;

當在執行交換位置的過程中，如果沒有交換過任何一次，則執行 `Switch to insertion sort`，還沒確定為什麼要這項;


計算出 qick sort 左右兩個 block 的 size，當左右的 size 皆大於設定的 forkelem 時，取得另一個 thread 並協助計算 qick sort，沒有的話，則當左邊 block size 大於 0 時，以遞迴的方式執行下一回合;
接著如果右邊的 block size 大於 0 時，變更 a 的 pointer 位置以及修改 n 的長度為右邊的 block 位置和長度，接著以 goto 的方式執行一下回合;
```

allocate_thread:
```
掃描整個 thread pool ，當有狀態為 idle 時，將它變成 work ，並返回該 thread
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
