# log-hook-contract

家族各公共库共用的中立日志挂钩契约：只含挂钩的类型定义（C 头文件 `lhc.h`，前缀 `lhc_`），**不含任何实现**。

- 为什么单独成仓：各库依赖这份契约，而不依赖任何具体的日志库实现；默认实现是 DryAlligator（旱地），由应用组合根把各库的挂钩接上去。换掉日志实现，各库零改动。
- 内容：字符串挂钩 v1；结构化挂钩 v2（级别、tag、格式串、类型化参数、键值）。v2 冻结前须具备：参数级 privacy 标志位、`is_enabled(level, tag)` 查询、日志实现导出的 C 函数表（供原生层直接接线）。
- 版本：独立版本线，按封版纪律只增不改——新增函数 / 结构体，不改既有签名。各 C++ 引擎经 CMake `FetchContent` 钉定 tag 拉取，不用浮动分支。
- 状态：v1.0 草案已实现（`include/lhc.h`），未冻结；冻结前待拍板项见 docs/abi-revisions.md。

## 使用

```c
#include "lhc.h"

/* 组合根注入 const lhc_sink* sink；消费方每次打日志前： */
if (lhc_sink_compatible(sink)) {
  static const lhc_str tag = LHC_STR_INIT("net");
  if (sink->is_enabled(sink->ctx, LHC_LEVEL_WARN, tag)) {
    /* 构造 lhc_record（格式串 + 类型化参数，敏感参数打 LHC_ARG_PRIVATE）后：*/
    sink->write_record(sink->ctx, &record);
  }
}
```

CMake 消费方：`FetchContent` 钉定 tag 引入本仓，链接 `lhc::lhc`。本仓构建与测试：`cmake --workflow --preset msvc`（Windows）/ `linux-gcc` / `linux-clang`。冻结前待拍板项见 [docs/abi-revisions.md](docs/abi-revisions.md)。

设计文档在 vault（不进本仓）：`20-知识/项目记录/公共库/日志库/`，本契约见 09 篇 L10-2。

Licensed under the [Apache License 2.0](LICENSE).
