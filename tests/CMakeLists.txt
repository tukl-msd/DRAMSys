add_executable(tests_dramsys
    AddressDecoderTests.cpp
    b_transport/b_transport.cpp
    main.cpp
    test_utils.cpp
)

target_include_directories(tests_dramsys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(tests_dramsys PROPERTIES FOLDER tests)
target_link_libraries(tests_dramsys
    DRAMSys::DRAMSys
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(tests_dramsys
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DISCOVERY_TIMEOUT 60
)

if (DRAMSYS_BUILD_REGRESSION_TESTS)
    add_subdirectory(tests_regression)
endif ()
