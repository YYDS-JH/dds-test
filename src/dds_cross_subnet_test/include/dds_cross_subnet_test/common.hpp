#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace dds_test {

inline std::string make_message(const std::string& node_name, std::uint64_t seq) {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << node_name << " | seq=" << seq << " | "
      << std::put_time(std::gmtime(&t), "%FT%T");
  return oss.str();
}

inline bool parse_seq(const std::string& msg, std::uint64_t& seq) {
  auto pos = msg.find("seq=");
  if (pos == std::string::npos) return false;
  pos += 4;
  auto end = msg.find_first_not_of("0123456789", pos);
  auto num = msg.substr(pos, end - pos);
  if (num.empty()) return false;
  seq = std::stoull(num);
  return true;
}

class LossDetector {
public:
  explicit LossDetector(const std::string& name, int stats_interval_ms = 5000)
      : name_(name), stats_interval_ms_(stats_interval_ms) {}

  void on_receive(std::uint64_t seq) {
    if (first_) {
      first_ = false;
      last_seq_ = seq;
      expected_ = 1;
      received_ = 1;
      return;
    }

    if (seq > last_seq_ + 1) {
      auto gap = seq - last_seq_ - 1;
      lost_ += gap;
      std::printf("[%s] WARNING: packet loss detected! missed %lu messages (seq %lu -> %lu)\n",
                  name_.c_str(), gap, last_seq_, seq);
    }

    if (seq > last_seq_) {
      expected_ += (seq - last_seq_);
    }
    received_ += 1;
    last_seq_ = seq;

    int msg_interval = stats_interval_ms_ / 100;
    if (msg_interval > 0 && expected_ % msg_interval == 0) {
      std::printf("[%s] STATS: expected=%lu received=%lu lost=%lu (%.1f%% loss)\n",
                  name_.c_str(), expected_, received_, lost_,
                  100.0 * lost_ / expected_);
    }
  }

  std::uint64_t received_count() const { return received_; }
  std::uint64_t lost_count() const { return lost_; }
  std::uint64_t expected_count() const { return expected_; }

private:
  std::string name_;
  int stats_interval_ms_;
  std::uint64_t received_{0};
  std::uint64_t lost_{0};
  std::uint64_t expected_{0};
  bool first_{true};
  std::uint64_t last_seq_{0};
};

}  // namespace dds_test
