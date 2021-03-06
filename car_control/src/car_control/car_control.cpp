#include "car_control/car_control.h"
#include <geometry_msgs/Twist.h>
#include <vector>
#include <cmath>
#include <ros/ros.h>

using namespace cv;
using namespace std;

CarControl::CarControl()
    : _nh{"car_control"}, _configServer{_nh}
{
    _configServer.setCallback(boost::bind(&CarControl::configCallback, this, _1, _2));

    carPos.x = 165;
    carPos.y = 230;

    std::string speed_topic, steer_topic, control_topic;
    ROS_ASSERT(ros::param::get("/set_speed_topic", speed_topic));
    ROS_ASSERT(ros::param::get("/set_angle_topic", steer_topic));
    ROS_ASSERT(ros::param::get("/control_topic", control_topic));
    speed_publisher = _nh.advertise<std_msgs::Float32>(speed_topic, 1);
    steer_publisher = _nh.advertise<std_msgs::Float32>(steer_topic, 1);
    control_subscriber = _nh.subscribe(control_topic, 1, &CarControl::driveCallback, this);
}

void CarControl::configCallback(car_control::CarControlConfig &config, uint32_t level)
{
    kCU = config.kCU;
    PU = config.PU;
    // kP = config.P;
    // kI = config.I;
    // kD = config.D;
    kP = kCU / 1.7;
    kI = PU / 2.0;
    kD = PU / 8.0;
    carPos.x = config.carpos_x;
    carPos.y = config.carpos_y;
    minVelocity = config.min_velocity;
    maxVelocity = config.max_velocity;
}

void CarControl::driveCallback(const geometry_msgs::Twist &msg)
{
    std_msgs::Float32 angle_msg;
    std_msgs::Float32 speed_msg;

    float speed = msg.linear.x;
    float error = msg.angular.z * 180 / M_PI;

    error = -error;

    ROS_DEBUG("Recv speed = %.2f, steer = %.2f", speed, error);

    // carPos.x = 165;
    //PID controller
    if (fabs(t_kI) > 200) // 40 * 5
    {
        t_kI = 0;
    }
    t_kP = error;
    t_kI += error;
    t_kD = error - preError;
    angle_msg.data = (kP * t_kP + kI * t_kI + kD * t_kD) / 1000.;
    // speed.data = fabs(error) < 1 ? maxVelocity : (velocity - fabs(error) * 0.35);

    if (abs(angle_msg.data) < 40)
    {
        speed = maxVelocity * (1 - abs(angle_msg.data) / 40.0f);
        
    }
    else 
    {
        speed = minVelocity;
        if (abs(angle_msg.data) > 50)
        {
            t_kP = 0.0;
            t_kI = 0.0;
            t_kD = 0.0;
        }
    }

    ROS_DEBUG("Send speed = %.2f, steer = %.2f", speed, error);

    // speed.data = velocity;
    speed_msg.data = speed;
    preError = error;

    angle_msg.data = error;
    speed_msg.data = speed;// speed;

    steer_publisher.publish(angle_msg);
    speed_publisher.publish(speed_msg);
}
