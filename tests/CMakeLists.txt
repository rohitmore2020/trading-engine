include(GoogleTest)

add_executable(te_tests
    test_smoke.cpp
)

target_link_libraries(te_tests
    PRIVATE
        te_core
        GTest::gtest_main
)

gtest_discover_tests(te_tests)
