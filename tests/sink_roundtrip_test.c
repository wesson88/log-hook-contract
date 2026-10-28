/*
 * 函数表的兼容检查与「大小 + 版本」机制：
 *   1. lhc_sink_compatible 拒绝 NULL、主版本不符、表过小、必备成员缺失；
 *   2. 新消费方面对旧实现：LHC_SINK_HAS 对旧表之外的成员返回假；
 *   3. 新实现面对旧消费方：LHC_RECORD_HAS 对旧记录之外的字段返回假；
 *   4. 记录与参数经函数表原样到达实现方（含隐私标记、键值、源码位置）。
 */
#include <stdio.h>
#include <string.h>

#include "lhc.h"

static int g_failures = 0;
#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
      ++g_failures;                                                    \
    }                                                                  \
  } while (0)

/* ---------------------------------------------------------------- 假实现方 */
typedef struct fake_sink_state {
  int text_calls;
  int record_calls;
  lhc_level last_level;
  char last_tag[32];
  char last_format[64];
  uint32_t last_arg_count;
  lhc_arg_flags last_arg_flags[4];
  int64_t last_i64;
  uint32_t last_kv_count;
  uint32_t last_line;
} fake_sink_state;

static void copy_str(char* dst, size_t cap, lhc_str s) {
  size_t n = s.len < cap - 1 ? s.len : cap - 1;
  if (n > 0) memcpy(dst, s.data, n);
  dst[n] = '\0';
}

static int32_t fake_is_enabled(void* ctx, lhc_level level, lhc_str tag) {
  (void)ctx;
  (void)tag;
  return level >= LHC_LEVEL_INFO ? 1 : 0;
}

static void fake_write_text(void* ctx, lhc_level level, lhc_str tag, lhc_str message) {
  fake_sink_state* st = (fake_sink_state*)ctx;
  (void)message;
  st->text_calls++;
  st->last_level = level;
  copy_str(st->last_tag, sizeof st->last_tag, tag);
}

static void fake_write_record(void* ctx, const lhc_record* r) {
  fake_sink_state* st = (fake_sink_state*)ctx;
  uint32_t i;
  st->record_calls++;
  st->last_level = r->level;
  copy_str(st->last_tag, sizeof st->last_tag, r->tag);
  copy_str(st->last_format, sizeof st->last_format, r->format);
  st->last_arg_count = r->arg_count;
  for (i = 0; i < r->arg_count && i < 4; ++i) st->last_arg_flags[i] = r->args[i].flags;
  if (r->arg_count > 0 && r->args[0].type == LHC_ARG_I64) st->last_i64 = r->args[0].value.i64;
  st->last_kv_count = r->kv_count;
  /* 实现方读取 source 之前按「大小」检查——v1.0 字段，旧记录也一定有 */
  if (LHC_RECORD_HAS(r, source) && r->source != NULL) st->last_line = r->source->line;
}

static lhc_sink make_sink(fake_sink_state* st) {
  lhc_sink s;
  memset(&s, 0, sizeof s);
  s.struct_size = (uint32_t)sizeof(lhc_sink);
  s.abi_version = LHC_ABI_VERSION;
  s.ctx = st;
  s.is_enabled = fake_is_enabled;
  s.write_text = fake_write_text;
  s.write_record = fake_write_record;
  return s;
}

/* ---------------------------------------------------------------- 1. 兼容检查 */
static void test_compatible(void) {
  fake_sink_state st;
  lhc_sink s;
  memset(&st, 0, sizeof st);
  s = make_sink(&st);
  CHECK(lhc_sink_compatible(&s));
  CHECK(!lhc_sink_compatible(NULL));

  {
    lhc_sink t = s;
    t.abi_version = (uint32_t)(2u << 16);  /* 主版本不符 */
    CHECK(!lhc_sink_compatible(&t));
  }
  {
    lhc_sink t = s;
    t.abi_version = LHC_ABI_VERSION + 7u;  /* 次版本更高：兼容 */
    CHECK(lhc_sink_compatible(&t));
  }
  {
    lhc_sink t = s;
    t.struct_size = (uint32_t)offsetof(lhc_sink, write_record);  /* 表过小 */
    CHECK(!lhc_sink_compatible(&t));
  }
  {
    lhc_sink t = s;
    t.write_record = NULL;  /* 必备成员缺失 */
    CHECK(!lhc_sink_compatible(&t));
    CHECK(!LHC_SINK_HAS(&t, write_record));
  }
}

/* ---------------------------------------------------------------- 2. 新消费方 × 旧实现
 * 模拟契约将来在表末尾追加了一个成员：新消费方按新布局编译，拿到的却是只有 v1.0 大小的旧表。 */
typedef void (*future_extra_fn)(void* ctx);
typedef struct future_sink {
  uint32_t struct_size;
  uint32_t abi_version;
  void* ctx;
  lhc_is_enabled_fn is_enabled;
  lhc_write_text_fn write_text;
  lhc_write_record_fn write_record;
  future_extra_fn extra;  /* v1.x 追加 */
} future_sink;
#define FUTURE_SINK_HAS(s, member) \
  ((size_t)(s)->struct_size >= offsetof(future_sink, member) + sizeof((s)->member) && (s)->member != NULL)

static void test_new_consumer_old_impl(void) {
  future_sink fs;
  memset(&fs, 0, sizeof fs);
  fs.struct_size = (uint32_t)LHC_SINK_V1_0_SIZE;  /* 旧实现只填到 v1.0 */
  fs.abi_version = LHC_ABI_VERSION;
  CHECK(offsetof(future_sink, write_record) == offsetof(lhc_sink, write_record));  /* 前缀布局一致 */
  CHECK(FUTURE_SINK_HAS(&fs, write_record) == 0);  /* 成员为 NULL → 不支持 */
  CHECK(FUTURE_SINK_HAS(&fs, extra) == 0);         /* 越出旧表 → 不支持，不得调用 */
  fs.struct_size = (uint32_t)sizeof(future_sink);
  CHECK(FUTURE_SINK_HAS(&fs, extra) == 0);         /* 表够大但成员为 NULL → 不支持 */
}

/* ---------------------------------------------------------------- 3. 新实现 × 旧消费方 */
static void test_new_impl_old_record(void) {
  lhc_record r;
  memset(&r, 0, sizeof r);
  r.struct_size = (uint32_t)sizeof(lhc_record);
  CHECK(LHC_RECORD_HAS(&r, source));
  r.struct_size = (uint32_t)offsetof(lhc_record, source);  /* 假想的更旧记录：没有 source 字段 */
  CHECK(!LHC_RECORD_HAS(&r, source));
  CHECK(LHC_RECORD_HAS(&r, kvs));
}

/* ---------------------------------------------------------------- 4. 往返 */
static void test_roundtrip(void) {
  fake_sink_state st;
  lhc_sink s;
  lhc_arg args[2];
  lhc_kv kv;
  lhc_source src;
  lhc_record r;
  const lhc_str tag = LHC_STR_INIT("net");
  const lhc_str fmt = LHC_STR_INIT("retry {} for {}");
  const lhc_str key = LHC_STR_INIT("attempt");
  const lhc_str user = LHC_STR_INIT("alice@example.com");
  const lhc_str file = LHC_STR_INIT("client.cpp");
  const lhc_str fn = LHC_STR_INIT("send");

  memset(&st, 0, sizeof st);
  s = make_sink(&st);
  CHECK(lhc_sink_compatible(&s));

  /* 消费方先问 is_enabled，关闭时跳过求值与构造 */
  CHECK(s.is_enabled(s.ctx, LHC_LEVEL_DEBUG, tag) == 0);
  CHECK(s.is_enabled(s.ctx, LHC_LEVEL_WARN, tag) != 0);

  memset(args, 0, sizeof args);
  args[0].type = LHC_ARG_I64;
  args[0].value.i64 = 3;
  args[1].type = LHC_ARG_STR;
  args[1].flags = LHC_ARG_PRIVATE;
  args[1].value.str = user;

  memset(&kv, 0, sizeof kv);
  kv.key = key;
  kv.value.type = LHC_ARG_U64;
  kv.value.value.u64 = 2;

  memset(&src, 0, sizeof src);
  src.file = file;
  src.function = fn;
  src.line = 42;

  memset(&r, 0, sizeof r);
  r.struct_size = (uint32_t)sizeof r;
  r.level = LHC_LEVEL_WARN;
  r.flags = LHC_RECORD_FORMAT_STATIC | LHC_RECORD_TAG_STATIC;
  r.tag = tag;
  r.format = fmt;
  r.args = args;
  r.arg_count = 2;
  r.kvs = &kv;
  r.kv_count = 1;
  r.source = &src;

  s.write_record(s.ctx, &r);
  CHECK(st.record_calls == 1);
  CHECK(st.last_level == LHC_LEVEL_WARN);
  CHECK(strcmp(st.last_tag, "net") == 0);
  CHECK(strcmp(st.last_format, "retry {} for {}") == 0);
  CHECK(st.last_arg_count == 2);
  CHECK(st.last_i64 == 3);
  CHECK(st.last_arg_flags[0] == 0);
  CHECK((st.last_arg_flags[1] & LHC_ARG_PRIVATE) != 0);
  CHECK(st.last_kv_count == 1);
  CHECK(st.last_line == 42);

  s.write_text(s.ctx, LHC_LEVEL_ERROR, tag, fmt);
  CHECK(st.text_calls == 1);
  CHECK(st.last_level == LHC_LEVEL_ERROR);
}

int main(void) {
  test_compatible();
  test_new_consumer_old_impl();
  test_new_impl_old_record();
  test_roundtrip();
  if (g_failures != 0) {
    fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
  }
  printf("sink_roundtrip_test: all checks passed\n");
  return 0;
}
