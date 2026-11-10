cmake_minimum_required(VERSION 3.21)
project(ThroneOfflineTrafficTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

get_filename_component(
    _throne_default_source_dir
    "${CMAKE_CURRENT_LIST_DIR}/../../.."
    ABSOLUTE
)
set(THRONE_SOURCE_DIR "${_throne_default_source_dir}" CACHE PATH "Path to the Throne repository root")

if(NOT EXISTS "${THRONE_SOURCE_DIR}/src/stats/traffic/TrafficLooper.cpp")
    message(FATAL_ERROR "THRONE_SOURCE_DIR does not point to the Throne repository: ${THRONE_SOURCE_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

enable_testing()

add_executable(throne_offline_traffic_tests
    TrafficRegressionTests.cpp
    "${THRONE_SOURCE_DIR}/include/engine/services/StatsService.h"
    "${THRONE_SOURCE_DIR}/src/engine/services/StatsService.cpp"
    "${THRONE_SOURCE_DIR}/src/stats/traffic/TrafficLooper.cpp"
)

# The forwarding headers replace only external facilities. The real SUT headers
# and both production implementation files are still compiled unchanged.
target_include_directories(throne_offline_traffic_tests PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/stubs"
    "${THRONE_SOURCE_DIR}"
)
target_link_libraries(throne_offline_traffic_tests PRIVATE Qt6::Core)

if(WIN32)
    add_custom_command(TARGET throne_offline_traffic_tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Qt6::Core>"
            "$<TARGET_FILE_DIR:throne_offline_traffic_tests>"
    )
endif()

add_test(NAME throne_offline_traffic_regression COMMAND throne_offline_traffic_tests)
set_tests_properties(throne_offline_traffic_regression PROPERTIES TIMEOUT 15)
