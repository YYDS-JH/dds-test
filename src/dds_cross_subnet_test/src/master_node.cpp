#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "dds_cross_subnet_test/common.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("master_node");
  auto detector = std::make_shared<dds_test::LossDetector>("master_node");

  auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();

  auto publisher =
      node->create_publisher<std_msgs::msg::String>("/master_to_slave", qos);

  auto subscription =
      node->create_subscription<std_msgs::msg::String>(
          "/slave_to_master", qos,
          [detector](std_msgs::msg::String::ConstSharedPtr msg) {
            std::uint64_t seq = 0;
            if (dds_test::parse_seq(msg->data, seq)) {
              detector->on_receive(seq);
            }
          });

  std::uint64_t seq = 0;
  auto timer =
      node->create_wall_timer(std::chrono::milliseconds(100), [&]() {
        auto msg = std_msgs::msg::String();
        msg.data = dds_test::make_message("master_node", seq++);
        publisher->publish(msg);
      });

  RCLCPP_INFO(node->get_logger(), "master_node started");
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
