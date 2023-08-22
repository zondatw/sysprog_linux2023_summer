# Priority inversion test

## 簡介

利用 `chrt -m` 來看所有的排程策略以及對應的最高和最低優先權
```shell
$ chrt -m
SCHED_OTHER min/max priority    : 0/0
SCHED_FIFO min/max priority     : 1/99
SCHED_RR min/max priority       : 1/99
SCHED_BATCH min/max priority    : 0/0
SCHED_IDLE min/max priority     : 0/0
SCHED_DEADLINE min/max priority : 0/0
```

## 執行

```shell
$ make check
cc -std=c11 -Wall -g -O2 -D_GNU_SOURCE -fsanitize=thread -DUSE_PTHREADS main.c -o test_pthread -lpthread
cc -std=c11 -Wall -g -O2 -D_GNU_SOURCE -fsanitize=thread -DUSE_PTHREADS -DFIX_PI main.c -o test_pthread_fix_pi -lpthread
Running test_pthread ...
Thread low func
Thread low func: ready to sleep
Thread mid func
Thread mid func: ready to sleep
Thread high func
Thread mid func: ready to sub
Thread mid func: execution [0]
Thread low func: execution [0]
Thread high func: ready to sleep
Thread high func: execution [1]
[OK]
Running test_pthread_fix_pi ...
pthread mutex: PTHREAD_PRIO_INHERIT
pthread mutex: PTHREAD_PRIO_INHERIT
Thread low func
Thread low func: ready to sleep
Thread mid func
Thread mid func: ready to sleep
Thread high func
Thread low func: execution [0]
Thread high func: ready to sleep
Thread high func: execution [1]
Thread mid func: ready to sub
Thread mid func: execution [0]
[OK]
```
