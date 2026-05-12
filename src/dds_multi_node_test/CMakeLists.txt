cmake_minimum_required(VERSION 3.8)
project(dds_multi_node_test)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(multi_node src/multi_node.cpp)
ament_target_dependencies(multi_node rclcpp std_msgs)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(TARGETS multi_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_lint_common REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
