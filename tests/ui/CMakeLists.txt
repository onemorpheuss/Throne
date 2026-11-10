cmake_minimum_required(VERSION 3.21)
project(ThroneOfflineUiTests LANGUAGES CXX)

get_filename_component(_throne_source_dir "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(THRONE_SOURCE_DIR "${_throne_source_dir}" CACHE PATH "Path to the Throne repository root")
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
enable_testing()

add_executable(ConnectionsTreeModelTests
    "${CMAKE_CURRENT_LIST_DIR}/ConnectionsTreeModelTests.cpp"
    "${THRONE_SOURCE_DIR}/src/ui/utils/ConnectionsTreeModel.cpp"
    "${THRONE_SOURCE_DIR}/include/ui/utils/ConnectionsTreeModel.h"
)
set_property(TARGET ConnectionsTreeModelTests PROPERTY AUTOMOC ON)
target_include_directories(ConnectionsTreeModelTests PRIVATE "${THRONE_SOURCE_DIR}")
target_link_libraries(ConnectionsTreeModelTests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME ConnectionsTreeModelTests COMMAND ConnectionsTreeModelTests)

add_executable(RecentIdOrderTests
    "${CMAKE_CURRENT_LIST_DIR}/RecentIdOrderTests.cpp"
    "${THRONE_SOURCE_DIR}/include/ui/utils/RecentIdOrder.h"
)
set_property(TARGET RecentIdOrderTests PROPERTY AUTOMOC ON)
target_include_directories(RecentIdOrderTests PRIVATE "${THRONE_SOURCE_DIR}")
target_link_libraries(RecentIdOrderTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME RecentIdOrderTests COMMAND RecentIdOrderTests)
