# 55 · 实战 · `pgo`（Profile-Guided Optimization）

> **TMA 分类**：Misc / 编译器层 · **加速比**：工程文档 **~1x**（本实验特殊）· **本机未实跑**（3 阶段构建复杂）

---

## 一、读题（工程侧背景）

**任务**：编译并运行 **Lua 解释器** 处理一段脚本 → 用**真实运行**的 profile 反馈给编译器，让它做更聪明的决策。

**PGO 三步流程**（工程 `pgo/` 目录里的四个 build 子目录对应）：
```
build_instr/   ← 用 -fprofile-instr-generate 编译的插桩版
      ↓ 跑代表性输入, 生成 default.profraw
build_baseline/ ← 普通 -O3 编译, 对照组
build_pgo_opt/  ← 用 -fprofile-use=<data> 二次编译, 依据真实 profile 优化
```

## 二、PGO 相比普通 -O3 多知道什么？

编译器在**没有 profile** 时只能"猜"：

| 决策点 | 猜（-O3）| 知道（PGO）|
|-------|---------|-----------|
| **内联大小阈值** | 保守（怕膨胀） | 只对**热路径**激进内联 |
| **分支布局** | 静态启发式 | **冷门分支外置**（cold section），让热路径连续 |
| **间接调用去虚化** | 只能 megamorphic 分派 | 观测到"95% 走同一个目标" → **内联 devirtualization + guard** |
| **基本块排序 / BBI** | 编译时猜 | 按真实执行顺序布局，减少 I-TLB / i-cache miss |
| **循环未提升** | 不敢 | 观测"从不抛异常" → 允许 hoisting |

## 三、为什么本实验文档说 ~1x？

Lua 解释器的核心是 **dispatch loop 的大 switch on opcode**：
- 解释器的分支已经通过 jump table / computed goto 布局
- 编译器很难通过 profile 再压缩它
- **PGO 更擅长"有明显冷热分支的常规代码"**（比如 if 里 99% 走 else，编译器能提前布局）
- Lua 的 workload 相对均匀 → PGO 收益就小

**PGO 收益强依赖 workload**：
- 大型 C++（V8、Chrome、LLVM 自身）：PGO **5-15%** 收益常见
- 数字密集 kernel（Lua 主循环、矩阵乘）：PGO 收益 0-3%
- 大量冷代码 + 少数热点：PGO 收益最高

## 四、本机为什么没实跑

PGO 需要**完整三步**（编译插桩 → 跑代表性 workload → 用 profile 重编），且 `pgo/CMakeLists.txt` 有特定 lua 子模块依赖。这个仓库里 4 个 build 子目录已存在，说明原作者做过，但**当前 shell 环境未预生成 .profraw** → 直接跑 `build_pgo_opt/bench` 需要重新走完流程。

**下次要复现**（clang 版）：
```bash
# 1. Instrument 编译
clang -O3 -fprofile-instr-generate=base.profraw -c *.cpp -o bin_instr
# 2. 跑代表性输入收集 profile
LLVM_PROFILE_FILE="%m.profraw" ./bin_instr < workload.lua
# 3. Merge + 用 profile 重编
llvm-profdata merge -output=lua.profdata *.profraw
clang -O3 -fprofile-use=lua.profdata -c *.cpp -o bin_pgo
```

## 五、PGO vs AutoFDO vs BOLT（现代全景）

| 技术 | 输入 | 特点 |
|------|------|------|
| **经典 PGO** | 插桩收集 profile | 需专门跑一遍, 有维护负担 |
| **AutoFDO** (Google) | 采样 perf LBR | **无插桩**，用生产环境数据 |
| **BOLT** (Meta) | 二进制后处理重排基本块 | 编译完再优化, 兼容任何编译器 |

现代大型项目（Chrome、LLVM）多用 **AutoFDO / BOLT**——避免"必须专门跑 workload 采集"的工程负担。

## 六、一句话收口

> **PGO = 让编译器"看着真实运行数据"再编一次**——它把"猜"变成"知道"（哪些分支走、哪些函数热、间接调用跳哪）。**收益强依赖 workload**：本实验 Lua 解释器几乎打满 switch → **~1x**；但真实大型 C++ 项目（Chrome/V8）能拿 5-15%。**它的最大教训：编译优化不是无脑开关，收益 = 编译器决策信息增量的体现**。
