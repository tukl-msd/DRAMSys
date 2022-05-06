add_executable(simpletest simpletest.cpp)
target_link_libraries(simpletest PRIVATE DRAMSysConfiguration nlohmann_json::nlohmann_json)

add_executable(converter converter.cpp)
target_link_libraries(converter PRIVATE DRAMSysConfiguration nlohmann_json::nlohmann_json)
