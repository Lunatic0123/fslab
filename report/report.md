# FSLab 报告

姓名 学号

## 测试结果

把 `python tests/test.py --release` 的最后一段输出粘贴到这里。

```
Trace 00 passed [  0.0022s] [ref:   0.0016s] [74.05855%]
Trace 01 failed
Trace 02 failed
Trace 03 failed
Trace 04 failed
Trace 05 failed
Trace 06 failed
Trace 07 failed
Trace 08 failed
Trace 09 failed
Trace 10 failed
Trace 11 failed
Trace 12 failed
Trace 13 failed
Trace 14 failed
Trace 15 failed
Trace 16 failed
Trace 17 failed
Trace 18 failed
Trace 19 failed
Trace p0 failed
Trace p1 failed
1/22 traces passed (excluding 3 open traces)
Total points: 2.73/60 (1/22)
```

## 亮点

比如你支持的特别的功能，或者是性能优化，或者是任何你觉得值得一提的地方，
如果你觉得把具体的内容放到下一节里更好，你可以只在这里像写摘要一样提及即可

## 总体设计

- 介绍 inode 之类的你的实现中用到的数据结构
- 介绍你的文件系统的设计指标（即证明你的文件系统支持的容量，单文件的最大大小，支持的最大 inode 数量这些符合要求）
- 介绍你的代码实现（大体上，就是吹你怎么又清晰又简单又高效地优雅地写完这一坨代码）
- 介绍你的性能优化（如果你有的话），比较优化前后的性能，如果你不是在 ICS 的服务器上完成的测试，请把你的硬件和软件环境写出来
- 如果你有实现特别的功能，你需要自己构造一些测试用例来验证这个功能的正确性（请保证助教可以比较方便地复现你说的，比如提供脚本）

## 想法

- 你可以把你想到的，但是没实现的在这里提出，最好不要和总体设计写到一起，那里主要指真正实现了的

## 总结

比如，优点，缺点，你觉得还可以做的工作，你对这个实验的看法建议之类的

## 附录

### 开放测试点

请贴出每个开放测试点你的输出，并解释为什么会这么输出（即解释你的文件系统正确地实现了这些测试点）

### 内存泄漏测试

请把 `python tests/test.py` 的最后一段输出粘贴到这里，这个对应的编译选项应该带有内存泄漏检查。

## 思考题

### 问题1

请设计一个方案，在不查资料的情况下，用一些常见的命令，比如：

- `dd if=/dev/random of=file bs=1K count=1`
- `stat file`
- `ls -l file`

计算出（或估计出）你自己的电脑（WSL 或 MacOS）的文件系统的以下参数：

1. 最小分配的块大小是多少
2. 每个 inode 包含几个直接指针
3. 每个一级间接指针可以指向多少个块（并猜测一下间接指针指向的数据块里面是怎么存的）
4. 你刚好想到的其他可以测出来的参数（不要求）

> 如果你的电脑不方便实验，你可以假设你有一个 Linux 系统，并说一下大致的方案即可。
>
> 如果你感觉你的文件系统不是这种 inode + data block 的方式实现的，你可以把分析对象换成本次实验你自己的文件系统
