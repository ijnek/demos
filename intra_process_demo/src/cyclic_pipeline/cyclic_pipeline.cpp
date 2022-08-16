// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <cinttypes>
#include <memory>

#include "rclcpp/executors.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/node_options.hpp"
#include "intra_process_demo_msgs/msg/data.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;

class IncrementerPipe : public rclcpp::Node
{
public:
  explicit IncrementerPipe(const rclcpp::NodeOptions & options)
  : Node("pipe", options)
  {
    pub = this->create_publisher<intra_process_demo_msgs::msg::Data>("out", 10);
    sub = this->create_subscription<intra_process_demo_msgs::msg::Data>(
      "in", 10, std::bind(&IncrementerPipe::topic_callback, this, _1));
  }

  void publish_first_message(intra_process_demo_msgs::msg::Data::UniquePtr msg)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "Published first message with value:  %d, and address: 0x%" PRIXPTR,
      msg->value,
      reinterpret_cast<std::uintptr_t>(msg.get()));
    pub->publish(std::move(msg));
  }

private:
  void topic_callback(intra_process_demo_msgs::msg::Data::UniquePtr msg) const
  {
    RCLCPP_INFO(
      this->get_logger(),
      "Received message with value:         %d, and address: 0x%" PRIXPTR,
      msg->value,
      reinterpret_cast<std::uintptr_t>(msg.get()));
    RCLCPP_INFO(this->get_logger(), "  sleeping for 1 second...");

    if (!rclcpp::sleep_for(1s)) {
      return;  // Return if the sleep failed (e.g. on ctrl-c).
    }

    RCLCPP_INFO(this->get_logger(), "  done.");
    msg->value++;  // Increment the message's data.
    RCLCPP_INFO(
      this->get_logger(),
      "Incrementing and sending with value: %d, and address: 0x%" PRIXPTR,
      msg->value,
      reinterpret_cast<std::uintptr_t>(msg.get()));
    pub->publish(std::move(msg));  // Send the message along to the output topic.
  }

  rclcpp::Publisher<intra_process_demo_msgs::msg::Data>::SharedPtr pub;
  rclcpp::Subscription<intra_process_demo_msgs::msg::Data>::SharedPtr sub;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor executor;

  // Create a simple loop by connecting the in and out topics of two IncrementerPipe's.
  auto pipe1 = std::make_shared<IncrementerPipe>(
    rclcpp::NodeOptions()
    .arguments({"--ros-args", "-r", "__name:=pipe1", "-r", "in:=topic1", "-r", "out:=topic2"})
    .use_intra_process_comms(true));
  auto pipe2 = std::make_shared<IncrementerPipe>(
    rclcpp::NodeOptions()
    .arguments({"--ros-args", "-r", "__name:=pipe2", "-r", "in:=topic2", "-r", "out:=topic1"})
    .use_intra_process_comms(true));

  // Wait for subscriptions to be established to avoid race conditions.
  rclcpp::sleep_for(1s);

  // Publish the first message (kicking off the cycle).
  auto msg = std::make_unique<intra_process_demo_msgs::msg::Data>();
  msg->value = 42;
  pipe1->publish_first_message(std::move(msg));

  // Add the nodes to the executor, and spin them
  executor.add_node(pipe1);
  executor.add_node(pipe2);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}
