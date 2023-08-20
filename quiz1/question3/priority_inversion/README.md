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

執行結果每次都是低優先權的先執行完畢：

```shell
$ make check
cc -std=c11 -Wall -g -O2 -D_GNU_SOURCE -fsanitize=thread main.c -o test_pthread -lpthread
Running test_pthread ...
Thread low func: ready to sleep
Thread mid func: ready to sleep
Thread mid func: execution [-1]
Thread low func: execution [-1]
Thread high func: ready to sleep
Thread high func: execution [0]
[OK]
```

執行測試，結果出現每次都發生 priority inversion：

```shell
$ sudo python test.py
100 / 100 = 1.0
```
