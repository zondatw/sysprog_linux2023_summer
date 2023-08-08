# Test Beta

## Build

`gcc main.c -o main`

## Questions

```
延伸問題:

說明上述程式碼的運作原理
在 Linux 核心原始程式碼找出類似 align_up 的程式碼，並舉例說明其用法
```

### Questions 1: (說明上述程式碼的運作原理)

`(((sz + mask) / alignment) * alignment)`

alignment 對齊的情況下，mask 為一個 alignment 區塊的最大值，因此當 (sz + mask) / alignment 我們可以得到需要幾個 alignment 區塊，接著再乘上 alignment 後即得到實際 size 

MMMM = `(sz & (~mask)) + ((sz & mask) != 0) * (mask + 1);`

`(sz & (~mask))` 為去掉遮罩部份的值，以取得地板值; `(sz & mask) != 0)` 是為了判斷遮罩部份有沒有值，如果有將他乘上 `(mask + 1)` = alignment，結果只會有 0 / alignment;
這段前提需要 `if ((alignment & mask) == 0) {  /* power of two? */` 的判斷，因為前述的 mask 應用是以 2 進制方式去發想。
