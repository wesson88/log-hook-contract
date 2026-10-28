# log-hook-contract · 项目说明

家族公共库的中立日志挂钩契约：只有 `include/lhc.h`（类型定义），**不含任何实现**。实现方是日志库 DryAlligator（旱地，私有仓 `wesson88/dry-alligator`）；消费方是神行 / 铁面 / 密文 SDK / 推送库。

## 设计输入（vault，唯一设计来源）

vault root：`D:\MarkDown\memory\adam`。主文档：`20-知识\项目记录\公共库\日志库\日志库-09-接入契约与平台壳.md`

| 要做的事 | 先读 |
|---|---|
| 契约要具备什么、为何单独成仓 | 09 篇 L10-2 |
| 格式串 / 参数类型 / 字典与驻留 | 06 篇 L5-2 / L5-3 |
| 级别、参数级隐私标记（占位 / HMAC） | 04 篇 L6-5 |
| 版本线、只增不改、「大小 + 版本」头 | 07 篇 L7-1 / L7-3 |
| 命名（`lhc_` / `LHC_` / `lhc.h`） | 09 篇 L10-11 |

文档与代码冲突时以文档为准；要偏离先在对话里提出，拍板后先改文档再改代码。冻结前待拍板项在 `docs/abi-revisions.md`。

## 规则

- **只放类型定义**：函数指针类型、结构体、枚举、宏与极小的 `static inline` 检查函数；不放任何日志实现，也不引用任何具体日志库。
- **只增不改**：冻结后新能力只能在 `lhc_sink` / `lhc_record` 末尾追加、新增类型与枚举值；`tests/abi_layout_test.c` 的快照变化 = breaking change。
- **头文件零告警**：C11 与 C++17、MSVC `/W4 /WX` 与 gcc / clang `-Wpedantic -Wconversion -Werror` 下都必须干净（消费方会以最严格告警包含它）。
- 分支策略：直接 commit + push `main`；每步构建 + 测试全绿再提交。

## 构建

- 本机：`cmake --workflow --preset msvc`
- Linux：`cmake --workflow --preset linux-gcc` / `linux-clang`（CI 同）
