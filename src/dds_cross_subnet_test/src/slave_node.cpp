#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "dds_cross_subnet_test/common.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("slave_node");
  auto detector = std::make_shared<dds_test::LossDetector>("slave_node");

  auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();

  auto publisher =
      node->create_publisher<std_msgs::msg::String>("/slave_to_master", qos);

  auto subscription =
      node->create_subscription<std_msgs::msg::String>(
          "/master_to_slave", qos,
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
        msg.data = dds_test::make_message("slave_node", seq++);
        publisher->publish(msg);
      });

  RCLCPP_INFO(node->get_logger(), "slave_node started");
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
