# 52 · 实战 · `function_inlining_1`（qsort + 函数指针 vs std::sort + lambda）

> **TMA 一级**：Core_Bound（**call overhead + branch mispredict**）· **优化技术**：**函数内联**·**加速比**：**1.63x 本机** ≈ 1.63x 文档

---

## 一、读题（Step 0）

**任务**：`N=10000` 个 `{key1, key2}` 结构体排序。

**基线**：C 库 `qsort` + 函数指针 comparator
```c
void solution(std::array<S,N> &arr) {
    std::qsort(arr.data(), arr.size(), sizeof(S), compare_S);   // 函数指针
}
static int compare_S(const void *a, const void *b) {
    // 返回 -1/0/1, 通过 void* 强转 → 每次调用都有:
    //   1. 间接 call (jmp *reg)  → 分支预测器难猜目标
    //   2. CALL/RET 栈帧维护
    //   3. void* → S* 强转 + 逐字段访问
}
```

**优化**：`std::sort` + lambda
```cpp
void solution(std::array<S,N> &arr) {
    std::sort(arr.begin(), arr.end(), [](const S&a, const S&b) {
        return a.key1 < b.key1 || (a.key1 == b.key1 && a.key2 < b.key2);
    });
}
```
- **lambda 是模板参数**（`_Compare`）→ **编译器把 compare 内联到 sort 内部**
- 无 call/ret、无间接跳转、无 `void*` 强转

---

## 二、Step ①②③④ 本机实测

| | 基线 qsort | 优化 std::sort+lambda | 变化 |
|--|-----------|---------------------|------|
| **Google Bench** | **574 μs** | **353 μs** | **1.63x** ✅ |
| 每 iter 指令 | 3.94 M | 1.71 M | ↓ **2.3x** |
| 每 iter branch-miss | 72 K | 96 K | 略多（但代价更低）|
| 每 iter cycles | ~487 K | ~297 K | ↓ 1.64x |

**关键**：**指令数少了 2.3 倍**——qsort 每次 compare 都 CALL 到独立函数、传 `void*`、强转、返回 → **大量开销在 call 机制本身**。std::sort+lambda 把整个比较内联进排序循环，**每次 compare 只剩 3-5 条指令**。

---

## 三、🎯 内联为什么值 1.63x？拆解三层开销

C `qsort` 每次比较的额外代价：

| 开销 | 具体 |
|------|------|
| **函数调用机制** | CALL + RET + 参数压栈/寄存器保存 → ~5-10 cycles |
| **间接跳转 BTB miss** | 比较函数指针固定 → 现代 BTB 猜对概率高，但排序热循环里仍有 |
| **无法跨边界优化** | qsort 库在编译单元外，编译器**看不到 compare 内部** → 无法 SIMD、无法提前发射下一次比较 |

**std::sort + lambda**：
- 编译器**看得到 compare 的每一条指令**（模板展开 + 内联）→ 直接嵌进排序主循环
- 无 call/ret、无 `void*` 强转
- 甚至可以**同时比较多个元素**（虽然 sort 通常没这么做）

---

## 四、和 LTO（第 4 课）+ `#pragma inline` 的关系

**内联的三种层次**：
1. **同 TU 内联**：编译器默认对 static / 小函数做（std::sort 的 lambda 天然在此）
2. **跨 TU 内联**：需要 `-flto`（LTO 把 IR 汇合再决策，对应 `TMA/misc/lto/` 实验）
3. **禁用/强制**：`__attribute__((noinline))` / `always_inline`

**qsort 的问题**：即使开 LTO 也内联不了——因为 `qsort` 是 libc 里的**外部 C 函数**，接收**运行时**函数指针。C 标准库设计决定了这条鸿沟。

**std::sort 天生赢**：header-only 模板 → **总在同一 TU 展开** → 内联无障碍。这就是"**用 C++ STL 而非 C 库**"的一个非常具体的性能理由。

---

## 五、"函数内联"的两个副作用（呼应 7 号 § 二.4）

1. **代码膨胀**：内联把函数体复制到每个调用点，热代码多时 I-Cache / uop Cache 压力上升 → **frontend bound** 可能反噬
2. **寄存器压力**：内联后活变量集合更大 → 可能 spill 到栈

**这就是 1.63x 而不是 3-5x 的原因之一**——大 N 排序里，内联后的 sort 主循环本身很复杂（快排的递归 + partition 循环），继续内联可能收益饱和。

---

## 六、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | qsort 574 μs |
| ③ TMA 分诊 | 每 iter 4M 指令 + 每次 CALL 5-10 cycles → **call overhead + indirect branch** |
| ④ 开药 | **换 std::sort + lambda**（让编译器内联比较） |
| ⑤ 验证 | 1.63x + 指令数 ↓2.3x ✅ |

---

## 七、一句话收口

> **qsort 的函数指针 = 每次比较 5-10 周期的"税"**——std::sort+lambda 通过内联直接免票。1.63x 加速看起来平淡，但**"选 C++ STL 模板 API 而不是 C 函数指针 API"是一个纯代码风格决策、零算法改动就能白拿的收益**。这也是为什么现代 C++ 性能最佳实践反复说：**"能不用 function pointer 就不用"**（对应 40 号 warmup 会看到"选对数据结构/算法"的更大杠杆）。

## 八、延伸

- 相关：`04-编译器概述.md § 一 3`（Inline 是中端主力）、`05-程序编写优化.md § 四`（内联优化）
- 姊妹实验：**54 lto**（跨翻译单元内联，1.50x）——把内联红利延伸到跨 TU 场景
- 原始工程文档：[`docs/function_inlining_1.md`](../experiments/core_bound/function_inlining_1.md)
