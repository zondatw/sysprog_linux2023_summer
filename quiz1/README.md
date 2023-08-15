# Quiz 1

# Questions

```
延伸問題:

解釋上述程式碼運作原理，應該涵蓋測試方法、futex 使用方式、Linux 核心開發者針對 POSIX Threads 強化哪些相關 futex 實作機制等等
修改第 1 次作業的測驗 γ 提供的並行版本快速排序法實作，使其得以搭配上述 futex 程式碼運作
研讀〈並行程式設計: 建立相容於 POSIX Thread 的實作〉，在上述程式碼的基礎之上，實作 priority inheritance mutex 並確認與 glibc 實作行為相同，應有對應的 PI 測試程式碼
比照 skinny-mutex，設計 POSIX Threads 風格的 API，並利用內附的 perf.c (斟酌修改) 確認執行模式符合預期，過程中也該比較 glibc 的 POSIX Threads 效能表現
```