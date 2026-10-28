/* 以 C11 编译 lhc.h：契约头必须是纯 C 可用的，且在最严格告警下零告警。 */
#include "lhc.h"

_Static_assert(LHC_ABI_VERSION == 0x00010000u, "ABI 版本 1.0");
_Static_assert(LHC_SINK_V1_0_SIZE == sizeof(lhc_sink), "v1.0 函数表即当前全表");

/* 级别间隔编号：与日志库编号登记表一致，冻结后不得改值 */
_Static_assert(LHC_LEVEL_VERBOSE == 10 && LHC_LEVEL_DEBUG == 20 && LHC_LEVEL_INFO == 30 &&
               LHC_LEVEL_WARN == 40 && LHC_LEVEL_ERROR == 50 && LHC_LEVEL_FATAL == 60, "级别值");
_Static_assert(LHC_ARG_NULL == 0 && LHC_ARG_BOOL == 1 && LHC_ARG_I64 == 2 && LHC_ARG_U64 == 3 &&
               LHC_ARG_F64 == 4 && LHC_ARG_STR == 5 && LHC_ARG_BYTES == 6 && LHC_ARG_EXCEPTION == 7, "参数类型值");
_Static_assert(LHC_ARG_PRIVATE == 1u && LHC_ARG_PRIVATE_LINKABLE == 2u, "隐私标记位");
_Static_assert((LHC_ARG_PRIVATE | LHC_ARG_PRIVATE_LINKABLE) == ((LHC_ARG_PRIVATE | LHC_ARG_PRIVATE_LINKABLE) & LHC_ARG_FLAGS_PRIVACY_MASK),
               "已定义的隐私标记都落在隐私类位段内");
_Static_assert(LHC_RECORD_FORMAT_STATIC == 1u && LHC_RECORD_TAG_STATIC == 2u, "记录标志位");

static const lhc_str k_tag = LHC_STR_INIT("net");

int main(void) {
  return k_tag.len == 3 ? 0 : 1;
}
