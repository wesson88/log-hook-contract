# lhc.h ABI 修订记录

规则（设计文档 L7-1 / L7-3、L10-2）：**M0 冻结、首次对外发布封版**。冻结后改动须实现方（日志库）与各消费方（神行 / 铁面 / 密文 SDK / 推送库）评审，并在本文件留一条记录；封版后只增不改——新能力 = 在 `lhc_sink` / `lhc_record` 末尾追加成员、新增类型与枚举值。

设计 SSOT：vault `20-知识/项目记录/公共库/日志库/日志库-09-接入契约与平台壳.md` L10-2（另见 06 篇 L5-2 / L5-3、04 篇 L6-5、07 篇 L7）。

## 当前状态

| 版本 | 状态 | 说明 |
|---|---|---|
| v1.0 | 草案（未冻结；D1–D14 已拍板） | `lhc_sink` 函数表 + 文本 / 结构化两种挂钩 + `is_enabled`；M0 出口时冻结 |

## 草案默认项（2026-10-28 用户拍板：D1–D14 全部按草案）

设计文档定了契约要具备什么，没写到实现粒度；以下按建议值实现，**2026-10-28 用户裁定全部按草案**，已登记回 vault 09 篇 L10-2「实现登记」。此后改动任一项须按冻结规则评审并在本文件留记录。

| # | 项 | 草案取值 |
|---|---|---|
| D1 | 挂钩形态 | 实现方提供 `lhc_sink` C 函数表（`struct_size` + `abi_version` + `ctx` + 成员函数指针），消费方只持有组合根注入的 `const lhc_sink*`——即 L10-2 要求的「日志库导出的 C 函数表」，原生库可在原生层直接接线 |
| D2 | 「字符串挂钩 v1 / 结构化挂钩 v2」的落法 | 头文件里命名为 `write_text` / `write_record`，**两者都在契约 ABI v1.0 中**；设计文档的 v1 / v2 指挂钩代际，不是本契约的 ABI 版本号 |
| D3 | v1.0 必备成员 | `is_enabled` / `write_text` / `write_record` 三者都不得为 NULL；`lhc_sink_compatible` 检查非 NULL + 主版本一致 + 尺寸足够；不兼容 = 不打日志、不报错、不崩溃 |
| D4 | 级别 | 间隔编号 10 / 20 / … / 60（verbose…fatal），与日志库编号登记表一致；未知级别按「不小于它的最近已知级别」处理，大于 fatal 按 fatal |
| D5 | 参数类型 | null / bool / i64 / u64 / f64 / str / bytes / exception（type + message + stack）；未知类型不读 value、渲染占位 |
| D6 | 隐私标记 | `PRIVATE`（占位，原值不留存，含崩溃残留）、`PRIVATE_LINKABLE`（带密钥 HMAC，隐含 PRIVATE；实现方不支持则退为 PRIVATE）；**位 0–7 保留给隐私类，未知隐私位一律按 PRIVATE**（宁可多遮）；位 8 起不认识则忽略 |
| D7 | 格式串 | `{}` 占位、`{{` / `}}` 转义；占位多于参数按缺参渲染，参数多于占位附在末尾 |
| D8 | 记录标志 | `FORMAT_STATIC` / `TAG_STATIC`：指针指向进程存活期不变的字面量，实现方可按地址驻留（对应 L5-3「调用点按字符串地址缓存」） |
| D9 | 文本挂钩与隐私 | `write_text` 的整段文本无法标记隐私——契约要求消费方不得在其中放敏感内容 |
| D10 | 字符串 | `lhc_str` 按值传递（两个字长），UTF-8 + 显式长度，不要求 NUL 结尾；`data` 为 NULL 当且仅当 `len == 0` |
| D11 | 布局 | 只用 32 位整数与指针 / `size_t`，显式 `reserved0` 填充；`abi_layout_test` 冻结 64 位与 32 位两套快照（32 位已在 i386 / ARM32 / wasm32 验证一致，含 i386 的 4 字节 int64 对齐） |
| D12 | 线程与生命周期 | 成员函数任意线程可并发调用、不阻塞调用方、不回调消费方、不让异常跨边界；参数只在调用期间有效；**不承诺异步信号安全**（与日志库 L3-1 一致） |
| D13 | `is_enabled` 语义 | 只是提示，可能随配置变化；返回 0 后仍调用 `write_*` 不算错误 |
| D14 | C++ 便捷封装 | 不提供（契约只含类型定义）；各引擎接入时手写参数数组。将来如需 `lhc.hpp`（变参模板生成参数数组 + 惰性求值）作为增量另议 |

## 消费方接入（回写各库时按此）

各公共库按其封版纪律**新增**一个 setter 接收 `const lhc_sink*`（不改既有日志 SPI，两版并存，L7-2 #5）；库内级别映射到 `LHC_LEVEL_*`，库特有的关联信息（如神行的请求 id / trace id）作为键值字段传入。各引擎经 CMake `FetchContent` 钉定本仓 tag 引入，链接 `lhc::lhc`。

## 验证

- 本机：`cmake --workflow --preset msvc`（4 项测试）。
- CI：`.github/workflows/verify.yml` 跑 `linux-gcc` / `linux-clang`。
- 32 位与交叉平台快照：x86_64 / i386 / ARM32 / arm64 Linux、wasm32-wasi、x86_64 Windows（mingw）以 clang `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion` 编译全部测试零告警（2026-10-28）。
