# m5stack-fidgets

M5Stack Core2 上的物理模擬小玩具合集:傾斜裝置給重力、點螢幕互動,撞擊有合成的鋼琴音與震動。

## 遊戲

| # | 名稱 | 玩法 |
|---|---|---|
| 1 | 彈珠 | 18 顆球互撞、殘影;手指排斥球 |
| 2 | 高爾頓板 | 自動落球堆成長條圖;點一下在該處丟 5 顆 |
| 3 | 色層擴張 | 旋轉的 6 色環,球撞到哪色那色往外長一層,先長滿的贏後整環脫落 |
| 4 | 井字 | 旋轉的 3x3 箱,牆和 O/X 都會反彈;每 2 秒在離球最近的空格蓋記號 |
| 5 | 切割 | 多邊形掃過中央小球就被切開,片片慢慢長大,塞滿後炸開 |

## 操作

螢幕下方三個觸控鍵:

- 選單:A / C 或左右滑換遊戲,B 或點預覽進入;10 秒沒操作自動輪播
- 遊戲內:A / C 切上一個 / 下一個,B 短按回選單、長按重置

## 建置

```
pio run -t upload
```

PlatformIO + Arduino framework,依賴 M5Unified(`platformio.ini`)。

## 校正

`src/common.h` 頂部:`TILT_X` / `TILT_Y` 傾斜方向反了就翻號,`TILT_GAIN` 調靈敏度。

## 結構

- `src/main.cpp`:選單與主迴圈;新遊戲寫一個 `.h` 提供 `init / step / draw`,在 `games[]` 加一行
- `src/common.h`:畫布、Ctx、顏色、震動、音效合成
- `src/ringlib.h`:環系列共用(弧線、球與扇區碰撞、火花、脫落環片)
- `src/balls.h` `plinko.h` `expand.h` `ttt.h` `slicer.h`:各遊戲
