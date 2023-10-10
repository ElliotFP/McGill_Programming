#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Bool.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

class Base_Controller
{
private:
    // Ros Variables
    ros::NodeHandle nh;
    ros::Subscriber twist_sub;
    ros::Subscriber pose_sub;
    ros::Subscriber done_sub;
    ros::Subscriber husky_pose_sub;
    ros::Publisher twist_pub;

    // Variables
    bool planner_done = true;
    bool moving = false;
    bool first = true;

    double t_x, t_y;
    double max_lin, max_ang;
    double v_x, v_y;
    double v_dot, z_dot;

public:
    Base_Controller(ros::NodeHandle nh)
    {
        nh.getParam("/husky_velocity_controller/linear/x/max_velocity", max_lin);
        nh.getParam("/husky_velocity_controller/angular/z/max_velocity", max_ang);

        twist_sub = nh.subscribe("planner/twist", 10, BaseController::twistCallback, this);
        pose_sub = nh.subscribe("planner/pose", 10, &BaseController::poseCallback, this);
        done_sub = nh.subscribe("planner/done", 10, &BaseController::doneCallback, this);
        husky_pose_sub = nh.subscribe("/husky_velocity_controller/feedback/pose", 10, &Base_Controller::huskyposeCallback, this);
        twist_pub = nh.advertise<geometry_msgs::Twist>("/husky_velocity_controller/cmd_vel", 10);
    }

    void twistCallback(const geometry_msgs::Twist::ConstPtr &twist)
    {
        moving = true;
        v_x = msg->linear.x;
        v_y = msg->linear.y;

        return;
    }

    void poseCallback(const geometry_msgs::Pose::ConstPtr &pose)
    {
        latest_pose_ = *pose;
    }

    void huskyposeCallback(const geometry_msgs::Pose::ConstPtr &pose)
    {
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);
        double x_r = (t_x - msg->position.x);
        double y_r = (t_y - msg->position.y);

        // Construct Jacobian
        Eigen::Matrix2d J;
        J(0, 0) = cos(yaw);
        J(0, 1) = -(sin(yaw) * x_r) - (cos(yaw) * y_r);
        J(1, 0) = sin(yaw);
        J(1, 1) = (cos(yaw) * x_r) - (sin(yaw) * y_r);

        // Take inverse and multiply by velocity vector
        Eigen::Vector2d v;
        v(0) = v_x;
        v(1) = v_y;
        Eigen::Vector2d w = J.inverse() * v;
        vdot = w(0);
        zdot = w(1);

        // Normalize
        if (v_dot > max_x)
        {
            vdot = max_x;
        }
        if (z_dot > max_z)
        {
            z_dot = max_z;
        }
        if (first)
        {
            ROS_DEBUG("vx: %f, vy: %f", v_x, v_y);
            ROS_DEBUG("vdot: %f, zdot: %f", v_dot, z_dot);
            first = false;
        }
        // Publish twist
        publish_twist();
        return;
    }

    void doneCallback(const std_msgs::Bool::ConstPtr &done_msg)
    {
        if (msg->data == true)
        {
            done = true;
            moving = false;
        }
        else
        {
            nh.getParam("/target/x", tx);
            nh.getParam("/target/y", ty);
            done = false;
        }
        return;
    }
    void publishTwist()
    {
        if (done)
        {
            moving = false;
            geometry_msgs::Twist twist;
            twist.linear.x = twist.linear.y = twist.linear.z = twist.angular.x = twist.angular.y = twist.angular.z = 0;
            twist_pub.publish(twist);
            return;
        }
        geometry_msgs::Twist twist;
        twist.linear.x = vdot;
        twist.linear.y = twist.linear.z = twist.angular.x = twist.angular.y = 0;
        twist.angular.z = zdot;
        twist_pub.publish(twist);
        return;
    }
};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "husky_velocity_controller");
    ros::NodeHandle nh;
    ros::Rate loop_rate(lr);

    Controller controller(nh);

    while (ros::ok())
    {
        ros::spinOnce();
        loop_rate.sleep();
    }
}
