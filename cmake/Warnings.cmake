# Warnings.cmake - compiler warnings interface target

add_library(te_warnings INTERFACE)

option(TE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CLANG_GCC_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wconversion
    -Wsign-conversion
    -Woverloaded-virtual
    -Wnull-dereference
    -Wformat=2
)

if(TE_WARNINGS_AS_ERRORS)
    list(APPEND CLANG_GCC_WARNINGS -Werror)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(te_warnings INTERFACE ${CLANG_GCC_WARNINGS})
endif()
