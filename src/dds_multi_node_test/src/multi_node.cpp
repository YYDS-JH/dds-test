#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

using namespace std::chrono_literals;

// ── helpers ────────────────────────────────────────────────────────────────

static std::string make_message(const std::string& name, std::uint64_t seq) {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << name << " | seq=" << seq << " | "
      << std::put_time(std::gmtime(&t), "%FT%T");
  return oss.str();
}

static bool parse_seq(const std::string& msg, std::uint64_t& seq) {
  auto pos = msg.find("seq=");
  if (pos == std::string::npos) return false;
  pos += 4;
  auto end = msg.find_first_not_of("0123456789", pos);
  auto num = msg.substr(pos, end - pos);
  if (num.empty()) return false;
  seq = std::stoull(num);
  return true;
}

// ── main ───────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("multi_node");
  node->declare_parameter<int>("node_index", 0);
  node->declare_parameter<int>("total_nodes", 6);
  node->declare_parameter<std::string>("node_name", "node_0");

  auto idx = node->get_parameter("node_index").as_int();
  auto total = node->get_parameter("total_nodes").as_int();
  auto name = node->get_parameter("node_name").as_string();
  auto names = node->declare_parameter("node_names", std::vector<std::string>{});

  auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();

  // Publisher: each node publishes its own topic
  auto pub_topic = "/n" + std::to_string(idx) + "_topic";
  auto publisher = node->create_publisher<std_msgs::msg::String>(pub_topic, qos);

  // Subscriptions: subscribe to all other nodes
  std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs;
  std::unordered_map<int, std::uint64_t> last_seq;

  for (int i = 0; i < total; ++i) {
    if (i == idx) continue;
    auto sub_topic = "/n" + std::to_string(i) + "_topic";
    auto sub = node->create_subscription<std_msgs::msg::String>(
        sub_topic, qos,
        [node, name, i, &names](std_msgs::msg::String::ConstSharedPtr msg) {
          std::uint64_t seq = 0;
          auto src_name = (i < (int)names.size()) ? names[i] : ("n" + std::to_string(i));
          if (parse_seq(msg->data, seq)) {
            RCLCPP_INFO(node->get_logger(),
                        "[%s] recv from %s seq=%lu", name.c_str(), src_name.c_str(), seq);
          } else {
            RCLCPP_INFO(node->get_logger(),
                        "[%s] recv from %s: '%s'", name.c_str(), src_name.c_str(),
                        msg->data.c_str());
          }
        });
    subs.push_back(sub);
  }

  // Timer: publish at 1Hz
  std::uint64_t seq = 0;
  auto timer = node->create_wall_timer(1s, [&]() {
    auto msg = std_msgs::msg::String();
    msg.data = make_message(name, seq++);
    publisher->publish(msg);
  });

  RCLCPP_INFO(node->get_logger(), "[%s] started on topic '%s' (n%ld of %ld)",
              name.c_str(), pub_topic.c_str(), idx, total);

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
