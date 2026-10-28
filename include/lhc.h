/*
 * lhc.h —— log-hook-contract：家族公共库的中立日志挂钩契约（只含类型定义，不含任何实现）。
 *
 * 版本：v1 草案。M0 冻结；首次对外发布封版，此后只增不改——新能力 = 在 lhc_sink /
 * lhc_record 末尾追加成员、新增类型与枚举值；既有成员的位置、类型、语义不变。
 * 冻结后改动须实现方与各消费方评审，并记入 docs/abi-revisions.md。
 *
 * 角色：
 *   - 实现方（日志库，如 DryAlligator）：提供一个填好的 lhc_sink（C 函数表）。
 *   - 消费方（各公共库）：只持有由应用组合根注入的 const lhc_sink*，经它打日志；
 *     不依赖任何日志实现，换掉实现零改动。原生库可在原生层直接接线，不经 JVM / Swift 往返。
 *
 * 兼容（「大小 + 版本」头）：
 *   - 消费方使用 sink 前须 lhc_sink_compatible(s)；不兼容即不打日志，不报错、不崩溃。
 *   - 调用 v1.0 之后追加的成员前须 LHC_SINK_HAS(s, 成员)；成员为 NULL 同样视为不支持。
 *   - 实现方读取 lhc_record 中 v1.0 之后追加的字段前须 LHC_RECORD_HAS(r, 字段)。
 *
 * 线程与生命周期（实现方须保证）：
 *   - 所有成员函数可在任意线程并发调用；不阻塞调用方（不做同步 I/O，不长时间持锁）。
 *   - 不回调消费方、不抛 C++ 异常跨越本边界、不终止进程；内部错误自行处理。
 *   - 一切参数（字符串、参数数组、记录）只在调用期间有效：返回前拷走所需内容。
 *   - sink 及其 ctx 在消费方最后一次使用前保持有效——组合根先关消费方、后关实现方。
 * 消费方须保证：
 *   - 不在异步信号处理函数中调用任何成员（本契约不承诺异步信号安全）。
 *   - 敏感参数只经 write_record 并打隐私标记传入；write_text 的整段文本无法标记，不得含敏感内容。
 *
 * 字符串：UTF-8 + 显式长度，不要求 NUL 结尾；data 可为 NULL 当且仅当 len == 0。
 */
#ifndef LHC_H
#define LHC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LHC_ABI_VERSION_MAJOR 1
#define LHC_ABI_VERSION_MINOR 0
/* (major << 16) | minor，写入 lhc_sink.abi_version。 */
#define LHC_ABI_VERSION ((uint32_t)(((uint32_t)LHC_ABI_VERSION_MAJOR << 16) | (uint32_t)LHC_ABI_VERSION_MINOR))

/* ------------------------------------------------------------------ 基础类型 */
typedef struct lhc_str {
  const char* data;
  size_t len;
} lhc_str;

typedef struct lhc_bytes {
  const uint8_t* data;
  size_t len;
} lhc_bytes;

/* 由字符串字面量初始化 lhc_str（C / C++ 通用的初始化器写法）：lhc_str t = LHC_STR_INIT("net"); */
#define LHC_STR_INIT(literal) { (literal), sizeof(literal) - 1 }

/* ------------------------------------------------------------------ 级别
 * 间隔编号（与日志库编号登记表一致），将来可在两级之间插入新级别。
 * 实现方遇到未知值：按不小于它的最近已知级别处理；大于 FATAL 按 FATAL。 */
typedef int32_t lhc_level;
enum {
  LHC_LEVEL_VERBOSE = 10,
  LHC_LEVEL_DEBUG = 20,
  LHC_LEVEL_INFO = 30,
  LHC_LEVEL_WARN = 40,
  LHC_LEVEL_ERROR = 50,
  LHC_LEVEL_FATAL = 60
};

/* ------------------------------------------------------------------ 参数
 * 格式串用 `{}` 作占位，按出现顺序依次取 args；`{{` 与 `}}` 表示字面花括号。
 * 占位多于参数时，缺的位置按「缺参」渲染；参数多于占位时，多余参数附在末尾。
 * 参数类型由 lhc_arg.type 携带，与占位符无关。 */
typedef uint32_t lhc_arg_type;
enum {
  LHC_ARG_NULL = 0,
  LHC_ARG_BOOL = 1,      /* value.boolean：0 / 非 0 */
  LHC_ARG_I64 = 2,       /* value.i64 */
  LHC_ARG_U64 = 3,       /* value.u64 */
  LHC_ARG_F64 = 4,       /* value.f64 */
  LHC_ARG_STR = 5,       /* value.str（UTF-8） */
  LHC_ARG_BYTES = 6,     /* value.bytes */
  LHC_ARG_EXCEPTION = 7  /* value.exception，不得为 NULL */
};
/* 实现方遇到未知 type：不读取 value，按「不认识的参数」渲染占位。 */

/* 隐私标记。位 0–7 保留给隐私类：实现方遇到不认识的隐私类位，一律按 LHC_ARG_PRIVATE 处理
 * （宁可多遮，不可漏遮）；位 8 起为其他用途，不认识则忽略。 */
typedef uint32_t lhc_arg_flags;
enum {
  /* 敏感：实现方须在任何落盘、输出、转发之前替换为占位，原值不得留存（含崩溃残留）。 */
  LHC_ARG_PRIVATE = 1u << 0,
  /* 敏感但需关联同一值：替换为带密钥的 HMAC（只在同一密钥下可比对），隐含 PRIVATE 的全部约束。
   * 实现方不支持 HMAC 时按 PRIVATE 处理。 */
  LHC_ARG_PRIVATE_LINKABLE = 1u << 1
};
#define LHC_ARG_FLAGS_PRIVACY_MASK 0xFFu

typedef struct lhc_exception {
  lhc_str type;
  lhc_str message;
  lhc_str stack;  /* 可为空 */
} lhc_exception;

typedef struct lhc_arg {
  lhc_arg_type type;
  lhc_arg_flags flags;
  union {
    int32_t boolean;
    int64_t i64;
    uint64_t u64;
    double f64;
    lhc_str str;
    lhc_bytes bytes;
    const lhc_exception* exception;
  } value;
} lhc_arg;

/* 键值字段（含作用域上下文）。key 为 UTF-8，value 的隐私标记同样生效。 */
typedef struct lhc_kv {
  lhc_str key;
  lhc_arg value;
} lhc_kv;

typedef struct lhc_source {
  lhc_str file;
  lhc_str function;
  uint32_t line;
  uint32_t reserved0;  /* 须为 0 */
} lhc_source;

/* ------------------------------------------------------------------ 结构化记录 */
typedef uint32_t lhc_record_flags;
enum {
  /* format.data 指向进程存活期内地址与内容都不变的字面量：实现方可按地址缓存（驻留）。 */
  LHC_RECORD_FORMAT_STATIC = 1u << 0,
  /* tag.data 同上。 */
  LHC_RECORD_TAG_STATIC = 1u << 1
};

typedef struct lhc_record {
  uint32_t struct_size;  /* 消费方填 sizeof(lhc_record)（按其编译时的契约版本） */
  lhc_level level;
  lhc_record_flags flags;
  uint32_t arg_count;
  uint32_t kv_count;
  uint32_t reserved0;    /* 须为 0 */
  lhc_str tag;
  lhc_str format;
  const lhc_arg* args;   /* arg_count == 0 时可为 NULL */
  const lhc_kv* kvs;     /* kv_count == 0 时可为 NULL */
  const lhc_source* source;  /* 可为 NULL */
  /* v1.0 之后只在此后追加字段 */
} lhc_record;

/* ------------------------------------------------------------------ 挂钩（C 函数表） */

/* 该级别与 tag 的日志当前是否会被记录：返回非 0 = 会。消费方据此跳过格式化与参数求值。
 * 结果只是提示，可能随配置变化；返回 0 后仍调用 write_* 不算错误。 */
typedef int32_t (*lhc_is_enabled_fn)(void* ctx, lhc_level level, lhc_str tag);

/* 文本挂钩：一段已拼好的文本。无法携带隐私标记，见文件头「消费方须保证」。 */
typedef void (*lhc_write_text_fn)(void* ctx, lhc_level level, lhc_str tag, lhc_str message);

/* 结构化挂钩：格式串 + 类型化参数 + 键值，参数可带隐私标记。 */
typedef void (*lhc_write_record_fn)(void* ctx, const lhc_record* record);

typedef struct lhc_sink {
  uint32_t struct_size;   /* 实现方填 sizeof(lhc_sink)（按其编译时的契约版本） */
  uint32_t abi_version;   /* 实现方填 LHC_ABI_VERSION */
  void* ctx;              /* 原样传回每个成员函数 */
  lhc_is_enabled_fn is_enabled;      /* v1.0 必备，不得为 NULL */
  lhc_write_text_fn write_text;      /* v1.0 必备，不得为 NULL */
  lhc_write_record_fn write_record;  /* v1.0 必备，不得为 NULL */
  /* v1.0 之后只在此后追加成员；追加的成员可为 NULL（= 不支持） */
} lhc_sink;

/* v1.0 函数表的最小尺寸。 */
#define LHC_SINK_V1_0_SIZE (offsetof(lhc_sink, write_record) + sizeof(lhc_write_record_fn))

/* 某成员是否落在实现方提供的表内且非 NULL。 */
#define LHC_SINK_HAS(s, member) \
  ((size_t)(s)->struct_size >= offsetof(lhc_sink, member) + sizeof((s)->member) && (s)->member != NULL)

/* 某字段是否落在消费方提供的记录内。 */
#define LHC_RECORD_HAS(r, field) \
  ((size_t)(r)->struct_size >= offsetof(lhc_record, field) + sizeof((r)->field))

/* 消费方使用 sink 前的检查：非 NULL、主版本一致、v1.0 必备成员齐全。 */
static inline int lhc_sink_compatible(const lhc_sink* s) {
  return s != NULL &&
         (s->abi_version >> 16) == (uint32_t)LHC_ABI_VERSION_MAJOR &&
         (size_t)s->struct_size >= LHC_SINK_V1_0_SIZE &&
         s->is_enabled != NULL && s->write_text != NULL && s->write_record != NULL;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* LHC_H */
