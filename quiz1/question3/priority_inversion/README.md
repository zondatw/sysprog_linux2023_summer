# Priority inversion test

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