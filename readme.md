# Head-Only可复用头文件类

## 介绍

这个仓库是用来创建存储可能会复用的头文件类。

## 大纲

- [Head-Only可复用头文件类](#head-only可复用头文件类)
  - [介绍](#介绍)
  - [大纲](#大纲)
  - [类介绍](#类介绍)
    - [EncodingConverter](#encodingconverter)
    - [DragArea](#dragarea)
    - [TimeCal](#timecal)
    - [Timer](#timer)
    - [AnimatedPushButton](AnimatedPushButton)



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

### TimeCal
使用CPP的高速定时器来实现计算代码运算耗时

### Timer
利用sleep_until函数来实现精准定时，比直接使用sleep_for有更高的精确度

### AnimatedPushButton

基于Qt的PushButton类实现的动画按钮类，实现了鼠标悬停动画、点击动画及鼠标离开动画。

可以通过调用函数轻松设置动画属性及按钮图片。

注：需要设置按钮图片才可以正常显示，该类就是为了按钮显示为图片而设计。