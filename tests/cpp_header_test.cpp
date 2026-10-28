// 以 C++17 编译 lhc.h：各家族引擎都是 C++，契约头在 C++ 下同样零告警。
#include "lhc.h"

#include <cstddef>
#include <type_traits>

static_assert(LHC_ABI_VERSION == 0x00010000u, "ABI 版本 1.0");
static_assert(LHC_SINK_V1_0_SIZE == sizeof(lhc_sink), "v1.0 函数表即当前全表");
static_assert(std::is_standard_layout<lhc_sink>::value && std::is_trivially_copyable<lhc_sink>::value,
              "函数表须为 C 布局");
static_assert(std::is_standard_layout<lhc_record>::value && std::is_trivially_copyable<lhc_record>::value,
              "记录须为 C 布局");
static_assert(std::is_standard_layout<lhc_arg>::value && std::is_trivially_copyable<lhc_arg>::value,
              "参数须为 C 布局");

namespace {
int32_t is_enabled(void*, lhc_level level, lhc_str) { return level >= LHC_LEVEL_INFO ? 1 : 0; }
void write_text(void*, lhc_level, lhc_str, lhc_str) {}
void write_record(void*, const lhc_record*) {}
}  // namespace

int main() {
  // C++ 侧以聚合初始化填表，与 C 侧同一份布局
  const lhc_sink sink = {static_cast<uint32_t>(sizeof(lhc_sink)), LHC_ABI_VERSION, nullptr,
                         &is_enabled, &write_text, &write_record};
  const lhc_str tag = LHC_STR_INIT("net");
  if (!lhc_sink_compatible(&sink)) return 1;
  if (!LHC_SINK_HAS(&sink, write_record)) return 2;
  if (sink.is_enabled(sink.ctx, LHC_LEVEL_DEBUG, tag) != 0) return 3;
  return 0;
}
