# Test alpha

## Build

`gcc main.c -o main`


## Questions

```
延伸問題:

解釋上述程式碼運作原理
指出上述程式碼可改進之處，特別跟 AVL tree 和 red-black tree 相比，並予以實作
設計效能評比程式，探討上述程式碼和 red-black tree 效能落差
```

### Question 1: (解釋上述程式碼運作原理)

主程式 （main）的流程為：
1. 初始化 treeint
2. 插入 n 個亂數到 tree 中
3. 以升序的方式將 tree 的內容顯示出來
4. 從 tree 中刪除 n 個亂數
5. 以升序的方式將 tree 的內容顯示出來
6. 清除 treeint

treeint_init 的行為：
```
初始化 tree 內容
```

treeint_insert 的行為：
```
從 root 開始查詢，有以下 3 種情境：
    1. 當 insert value: a 已經在 tree 中時返回該節點
    2. 當 a < t->value，改找 left 節點
    3. 當 a > t->value，改找 right 節點
直到沒有下一個節點為止，並且 d 會存儲 a 與 t->value 的關係，當 a < t->value
時 d 為 LEFT，當 a > t->value 時 d 為 RIGHT;
接著建立新節點，有以下 2 種情境：
    1. 當目前還沒有任何節點時，將 tree 的 root 指向新節點
    2. 或是將新節點根據 d 值插入到 p (前一個節點) 的 LEFT / RIGHT
返回新節點
```

treeint_find 的行為：
```
從 root 開始查詢，有以下 3 種情境：
    1. 當 insert value: a 已經在 tree 中時返回該節點
    2. 當 a < t->value，改找 left 節點
    3. 當 a > t->value，改找 right 節點
直到沒有下一個節點為止，沒有找到時返回 0;
```

treeint_remove 的行為：
```
先透過 a 查詢節點，當節點不存在時返回 -1，存在時從 tree 中移除節點
```

treeint_dump 的行為：
```
從 tree 的 root 開始查找並呼叫 __treeint_dump;
__treeint_dump 會將tree 的內容都顯示出來，因為是 ASC 的方式，因此從最小開始顯示，先顯示左節點再顯示右節點
```

treeint_destroy 的行為：
```
從 tree 的 root 以遞迴方式下去刪除，透過 __treeint_destroy 將 n 的節點依序先左後右，當節點存在時清除它
```
