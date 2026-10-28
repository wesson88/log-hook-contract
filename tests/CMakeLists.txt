# 契约头的编译检查与 ABI 布局快照。纯 C / C++ 可执行文件，不引第三方测试框架。
function(lhc_add_test name src)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE lhc::lhc)
  lhc_apply_compile_options(${name})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

lhc_add_test(c_header_test c_header_test.c)          # 纯 C11 可用
lhc_add_test(cpp_header_test cpp_header_test.cpp)    # C++17 可用
lhc_add_test(abi_layout_test abi_layout_test.c)      # 布局快照：冻结后任何偏移变化即失败
lhc_add_test(sink_roundtrip_test sink_roundtrip_test.c)  # 兼容检查与「大小 + 版本」机制
