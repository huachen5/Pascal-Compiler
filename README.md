Pascal-Compiler & Visualizer
一个基于 C++11 与 EasyX 图形库实现的高级程序设计语言（类 Pascal 文法）编译器与可视化调试分析系统。
本项目完整实现了从源码词法分析、语义语法分析、基于有向无环图（DAG）的中间代码基本块优化，到最终生成 8086 汇编目标代码的全套编译流程，并实时渲染出中间状态的内存栈映像。

核心特性
严谨的语言前端解析：基于 DFA 思想实现词法扫描器；采用递归下降子程序法解析复杂数据类型（支持 Record 记录、Array 数组）与多维复杂左值访问。
无限深度嵌套作用域管理：设计了支持动态生命周期的符号表（Symbol Table），逆序检索实现局部变量、值参、引用参数（ref）及函数作用域的精细化隔离与地址偏移量分配。
有向无环图（DAG）基本块优化：支持算术表达式的常数折叠（Constant Folding）与公共子表达式消除，严格遵循规范优化冗余临时变量赋值。
回填与目标代码生成：利用标准库栈结构精准解决结构化控制流（if-then-else / while-do）的拉链与回填难题；动态映射生成可运行的 8086 汇编指令。
全流程图形化调试面板：基于 EasyX Graphics 实时渲染词法二元组对照表、符号表、DAG 优化前后四元式对比，并动态绘制运行时活动记录（Activation Record）的栈映像。

项目结构
```text
├── Project1.slnx           # Visual Studio 解决方案引导文件
└── Project1/
    └── Project1/           # 核心源码文件夹
        ├── main.cpp        # EasyX UI 界面绘制与系统主控逻辑
        ├── Compiler.h      # 全局 Token、符号表项及四元式结构定义
        ├── Scanncer.cpp    # 词法分析器实现
        ├── Parser.cpp      # 语法分析与语义分析器（递归下降）
        ├── Optimizer.cpp   # DAG 中间代码优化引擎
        └── TargetGenerator.cpp # 8086 汇编代码生成器
