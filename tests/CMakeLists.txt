include(GoogleTest)

add_executable(te_tests
    test_smoke.cpp
    test_order.cpp
    test_validation.cpp
    test_order_book_basic.cpp
    test_invariants.cpp
    test_cancel_modify.cpp
    test_property_random.cpp
    test_stress.cpp
)

target_link_libraries(te_tests
    PRIVATE
        te_core
        GTest::gtest_main
)

gtest_discover_tests(te_tests)
