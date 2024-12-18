# Head-Only可复用头文件类

## 介绍

这个仓库是用来创建存储可能会复用的头文件类。

## 大纲

1. [EncodingConverter](###EncodingConverter)
2. [DragArea](###DragArea)



## 类介绍

### EncodingConverter

 EncodingConverter 提供自动检测文本文件编码并转换为目标编码的功能。

 支持通过正则表达式过滤文件和源编码。

使用 uchardet 来检测编码，ICU (ucnv) 来执行编码转换。

通过重写 protected 的 logMessage 方法，可以实现自定义日志输出。

### DragArea

DragArea是一个自定义的控件，用于在GUI中显示文件拖拽和文件夹选择操作。

它支持两种显示模式：

——FileSelectionMode：显示提示信息，可以拖拽文件或文件夹进来，并且点击可以打开文件夹选择对话框。

——LogOutputMode：显示日志信息，并在追加日志时自动滚动到底部。

用户可通过 setDisplayMode() 在两种模式之间切换。