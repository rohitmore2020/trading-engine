# Sanitizers.cmake - runtime sanitizers interface target

add_library(te_sanitizers INTERFACE)

set(TE_SANITIZER "none" CACHE STRING "Compile with sanitizers: none|address|thread")
set_property(CACHE TE_SANITIZER PROPERTY STRINGS none address thread)

if(TE_SANITIZER STREQUAL "address")
    message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
    target_compile_options(te_sanitizers INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(te_sanitizers INTERFACE
        -fsanitize=address,undefined
    )
elseif(TE_SANITIZER STREQUAL "thread")
    message(STATUS "Enabling ThreadSanitizer")
    target_compile_options(te_sanitizers INTERFACE
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(te_sanitizers INTERFACE
        -fsanitize=thread
    )
elseif(NOT TE_SANITIZER STREQUAL "none")
    message(FATAL_ERROR "Unknown sanitizer '${TE_SANITIZER}'. Choose from: none, address, thread")
endif()
