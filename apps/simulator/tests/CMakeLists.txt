find_package(GTest REQUIRED)

add_executable(tests_simulator
    main.cpp
    cache/tests_cache.cpp
    cache/TargetMemory.cpp
    cache/ListInitiator.cpp
)

set_target_properties(tests_simulator PROPERTIES FOLDER tests/simulator)
target_include_directories(tests_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_simulator PRIVATE
    DRAMSys::simulator
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(tests_simulator
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DISCOVERY_TIMEOUT 60
)
