#include <gtest/gtest.h>
#include <string>
#include <regex>

#include "dds_cross_subnet_test/common.hpp"

TEST(CommonTest, make_message_format) {
  auto msg = dds_test::make_message("master", 42);

  // Should contain node name and sequence number
  EXPECT_NE(msg.find("master"), std::string::npos);
  EXPECT_NE(msg.find("seq=42"), std::string::npos);

  // Should match pattern: "<name> | seq=<n> | <timestamp>"
  std::regex pattern(R"(^[^|]+ \| seq=\d+ \| .+$)");
  EXPECT_TRUE(std::regex_match(msg, pattern));
}

TEST(CommonTest, parse_seq_valid) {
  std::uint64_t seq = 0;
  EXPECT_TRUE(dds_test::parse_seq("slave | seq=7 | 2026-05-11T10:30:00", seq));
  EXPECT_EQ(seq, 7u);
}

TEST(CommonTest, parse_seq_malformed) {
  std::uint64_t seq = 0;
  EXPECT_FALSE(dds_test::parse_seq("garbage message", seq));
  EXPECT_FALSE(dds_test::parse_seq("", seq));
  EXPECT_FALSE(dds_test::parse_seq("seq=abc", seq));
}

// --- LossDetector tests ---

TEST(LossDetectorTest, sequential_no_loss) {
  dds_test::LossDetector detector("test", 5000);
  for (std::uint64_t i = 0; i < 10; ++i) {
    detector.on_receive(i);
  }
  EXPECT_EQ(detector.expected_count(), 10u);
  EXPECT_EQ(detector.received_count(), 10u);
  EXPECT_EQ(detector.lost_count(), 0u);
}

TEST(LossDetectorTest, detects_gap) {
  dds_test::LossDetector detector("test", 5000);
  detector.on_receive(0);
  detector.on_receive(1);
  detector.on_receive(5);  // gap: 2,3,4 lost

  EXPECT_EQ(detector.expected_count(), 6u);  // expected up to seq 5
  EXPECT_EQ(detector.received_count(), 3u);
  EXPECT_EQ(detector.lost_count(), 3u);
}

TEST(LossDetectorTest, first_message_any_seq) {
  dds_test::LossDetector detector("test", 5000);
  detector.on_receive(100);
  // First message sets baseline, no loss
  EXPECT_EQ(detector.expected_count(), 1u);
  EXPECT_EQ(detector.received_count(), 1u);
  EXPECT_EQ(detector.lost_count(), 0u);
}

TEST(LossDetectorTest, stats_interval) {
  // 300ms interval = stats every 3 expected messages
  dds_test::LossDetector detector("test", 300);
  for (std::uint64_t i = 0; i < 6; ++i) {
    detector.on_receive(i);
  }
  // After 6 sequential messages, expected=6, received=6, lost=0
  // Stats printed at expected_ % 3 == 0 (3 and 6)
  EXPECT_EQ(detector.expected_count(), 6u);
  EXPECT_EQ(detector.received_count(), 6u);
  EXPECT_EQ(detector.lost_count(), 0u);
}

TEST(LossDetectorTest, duplicate_ignored) {
  dds_test::LossDetector detector("test", 5000);
  detector.on_receive(0);
  detector.on_receive(1);
  detector.on_receive(1);  // duplicate
  detector.on_receive(2);

  // Duplicate incremented received but not expected
  EXPECT_EQ(detector.expected_count(), 3u);
  EXPECT_EQ(detector.received_count(), 4u);
  EXPECT_EQ(detector.lost_count(), 0u);
}
