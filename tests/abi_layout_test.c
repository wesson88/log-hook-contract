/*
 * ABI 布局快照。契约冻结后，下列偏移与尺寸任何变化即为 breaking change——测试失败时不得改快照，
 * 应改回布局（新能力只能在末尾追加，追加后更新尺寸快照并记入 docs/abi-revisions.md）。
 * 布局刻意只由 32 位整数与指针 / size_t 组成且显式填充，使各 32 位 ABI（含 i386 的 4 字节 int64 对齐）一致。
 */
#include <stddef.h>
#include <stdint.h>

#include "lhc.h"

#define OFF(type, field, p64, p32) \
  _Static_assert(offsetof(type, field) == (sizeof(void*) == 8 ? (p64) : (p32)), #type "." #field)
#define SIZE(type, p64, p32) _Static_assert(sizeof(type) == (sizeof(void*) == 8 ? (p64) : (p32)), "sizeof " #type)

_Static_assert(sizeof(void*) == 8 || sizeof(void*) == 4, "只支持 32 / 64 位");
_Static_assert(sizeof(size_t) == sizeof(void*), "size_t 与指针同宽");

OFF(lhc_str, data, 0, 0);
OFF(lhc_str, len, 8, 4);
SIZE(lhc_str, 16, 8);
SIZE(lhc_bytes, 16, 8);

OFF(lhc_arg, type, 0, 0);
OFF(lhc_arg, flags, 4, 4);
OFF(lhc_arg, value, 8, 8);
SIZE(lhc_arg, 24, 16);

OFF(lhc_exception, type, 0, 0);
OFF(lhc_exception, message, 16, 8);
OFF(lhc_exception, stack, 32, 16);
SIZE(lhc_exception, 48, 24);

OFF(lhc_kv, key, 0, 0);
OFF(lhc_kv, value, 16, 8);
SIZE(lhc_kv, 40, 24);

OFF(lhc_source, file, 0, 0);
OFF(lhc_source, function, 16, 8);
OFF(lhc_source, line, 32, 16);
OFF(lhc_source, reserved0, 36, 20);
SIZE(lhc_source, 40, 24);

OFF(lhc_record, struct_size, 0, 0);
OFF(lhc_record, level, 4, 4);
OFF(lhc_record, flags, 8, 8);
OFF(lhc_record, arg_count, 12, 12);
OFF(lhc_record, kv_count, 16, 16);
OFF(lhc_record, reserved0, 20, 20);
OFF(lhc_record, tag, 24, 24);
OFF(lhc_record, format, 40, 32);
OFF(lhc_record, args, 56, 40);
OFF(lhc_record, kvs, 64, 44);
OFF(lhc_record, source, 72, 48);
SIZE(lhc_record, 80, 52);

OFF(lhc_sink, struct_size, 0, 0);
OFF(lhc_sink, abi_version, 4, 4);
OFF(lhc_sink, ctx, 8, 8);
OFF(lhc_sink, is_enabled, 16, 12);
OFF(lhc_sink, write_text, 24, 16);
OFF(lhc_sink, write_record, 32, 20);
SIZE(lhc_sink, 40, 24);

int main(void) { return 0; }
