########################################
###          nlohmann_json           ###
########################################

project(nlohmann_json VERSION 3.11.2)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})