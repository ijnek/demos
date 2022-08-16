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

#include "rclcpp/node.hpp"
#include "rclcpp/executors.hpp"
#include "intra_process_demo_msgs/msg/data.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;

class Publisher : public rclcpp::Node
{
public:
  Publisher()
  : Node(("publisher"), rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    publisher_ = this->create_publisher<intra_process_demo_msgs::msg::Data>("number", 10);
    timer_ = this->create_wall_timer(
      1s, std::bind(&Publisher::timer_callback, this));
  }

private:
  void timer_callback()
  {
    auto msg = std::make_unique<intra_process_demo_msgs::msg::Data>();
    msg->stamp = rclcpp::Clock(RCL_SYSTEM_TIME).now();
    msg->value = count++;
    RCLCPP_INFO(
      this->get_logger(),
      "Published message with value: %d, and address: 0x%" PRIXPTR,
      msg->value,
      reinterpret_cast<std::uintptr_t>(msg.get()));
    publisher_->publish(std::move(msg));
  }

  rclcpp::Publisher<intra_process_demo_msgs::msg::Data>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  int32_t count = 0;
};

class Subscriber : public rclcpp::Node
{
public:
  Subscriber()
  : Node("subscriber", rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    subscription_ = this->create_subscription<intra_process_demo_msgs::msg::Data>(
      "number", 10, std::bind(&Subscriber::topic_callback, this, _1));
  }

private:
  void topic_callback(const intra_process_demo_msgs::msg::Data::UniquePtr msg) const
  {
    RCLCPP_INFO(
      this->get_logger(),
      " Received message with value: %d, and address: 0x%" PRIXPTR,
      msg->value,
      reinterpret_cast<std::uintptr_t>(msg.get()));
  }

  rclcpp::Subscription<intra_process_demo_msgs::msg::Data>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor executor;

  auto publisher = std::make_shared<Publisher>();
  auto subscriber = std::make_shared<Subscriber>();

  executor.add_node(publisher);
  executor.add_node(subscriber);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}
