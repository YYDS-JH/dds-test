cmake_minimum_required(VERSION 3.8)
project(dds_cross_subnet_test)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# Common library (header-only)
add_library(common INTERFACE)
target_include_directories(common INTERFACE include)

# Master node
add_executable(master_node src/master_node.cpp)
ament_target_dependencies(master_node rclcpp std_msgs)
target_link_libraries(master_node common)

# Slave node
add_executable(slave_node src/slave_node.cpp)
ament_target_dependencies(slave_node rclcpp std_msgs)
target_link_libraries(slave_node common)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(TARGETS master_node slave_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_lint_common REQUIRED)

  ament_lint_auto_find_test_dependencies()

  ament_add_gtest(test_common test/test_common.cpp)
  target_include_directories(test_common PRIVATE include)
  ament_target_dependencies(test_common rclcpp std_msgs)
endif()

ament_package()
