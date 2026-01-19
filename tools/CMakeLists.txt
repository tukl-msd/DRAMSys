find_package(nlohmann_json REQUIRED)

add_executable(json_converter json_converter.cpp)

target_link_libraries(json_converter PRIVATE
    DRAMSys::DRAMSys
    nlohmann_json::nlohmann_json
)
