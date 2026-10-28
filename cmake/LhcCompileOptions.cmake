# 本仓目标的编译选项：契约头必须在最严格的告警下零告警（消费方会以 -Werror / /WX 包含它）。
function(lhc_apply_compile_options target)
  if(MSVC)
    # /utf-8：本机代码页 936，源码为无 BOM UTF-8
    target_compile_options(${target} PRIVATE /utf-8 /W4 /WX /permissive- /Zc:__cplusplus)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion)
  endif()
endfunction()
