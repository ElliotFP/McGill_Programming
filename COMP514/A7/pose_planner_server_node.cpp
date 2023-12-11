#include <ros/ros.h>
#include <highlevel_msgs/MoveTo.h>
#include <geometry_msgs/PoseStamped.h>

class Planner
{
private:
    // ros variables
    ros::Publisher pose_pub;
    ros::Publisher twist_pub;
    ros::ServiceServer service;
    ros::Subscriber current_pose_sub;
    ros::Timer trajectory_timer;
    ros::NodeHandle nh;
    ros::RosActionClient client;

    // variables
    geometry_msgs::Pose current_pose;
    geometry_msgs::Pose current_twist;
    geometry_msgs::Pose desired_pose;
    geometry_msgs::Pose desired_twist;

    // polynomial coefficients
    double ax[4], ay[4], az[4];
    double start_time, end_time;

    // functions
    void computeCoefficients;
    void trajectoryCallback;
    void poseCallback;
    bool actioncallback;

public:
    Planner(ros::NodeHandle &node_handle) : nh(node_handle) // constructor
    {
        pose_pub = node_handle.advertise<geometry_msgs::Pose>("/gen3/reference/pose", 10);
        twist_pub = node_handle.advertise<geometry_msgs::Pose>("/gen3/reference/twist", 10);
        sub = node_handle.subscribe<geometry_msgs::Pose>("/gen3/joint_states", 10, &PosePlanner::poseCallback, this);
        client = node_handle.serviceClient<highlevel_msgs::PoseCommand>("/action_planner/pose_command"); // check if this is the right name

        // service = node_handle.advertiseService("/pose_planner/move_to", &PosePlanner::my_call_back, this);

        ROS_INFO("Server: Pose planner server is created.");
    }
};

void Planner::computeCoefficients(double start, double end, double T, double *a)
{
    a[0] = start;
    a[1] = 0;
    a[2] = 3 * (end - start) / (T * T);
    a[3] = -2 * (end - start) / (T * T * T);
}

void Planner::trajectoryCallback(const ros::TimerEvent &)
{
    double current_time = ros::Time::now().toSec();
    double t = current_time - start_time;
    if (current_time > end_time)
    {
        trajectory_timer.stop();
        return;
    }

    geometry_msgs::Pose target_pose;
    target_pose.position.x = ax[0] + ax[1] * t + ax[2] * t * t + ax[3] * t * t * t;
    target_pose.position.y = ay[0] + ay[1] * t + ay[2] * t * t + ay[3] * t * t * t;
    target_pose.position.z = az[0] + az[1] * t + az[2] * t * t + az[3] * t * t * t;

    desired_pose_pub.publish(target_pose);
}

void Planner::poseCallback(const geometry_msgs::Pose::ConstPtr &msg)
{
    current_pose = *msg;
}

bool Planner::actioncallback(highlevel_msgs::MoveTo::Request &req, highlevel_msgs::MoveTo::Response &res)
{
    ROS_INFO_STREAM("Received Target Pose: x=" << req.x << ", y=" << req.y << ", z=" << req.z << ", T=" << req.T);

    if (req.z <= 0 || req.T <= 0)
    {
        res.success = false;
        return false;
    }
    computeCoefficients(current_pose.position.x, req.x, req.T, ax);
    computeCoefficients(current_pose.position.y, req.y, req.T, ay);
    computeCoefficients(current_pose.position.z, req.z, req.T, az);

    start_time = ros::Time::now().toSec();
    end_time = start_time + req.T;
    trajectory_timer = nh.createTimer(ros::Duration(0.002), &PosePlanner::trajectoryCallback, this, false, true); // Start timer immediately

    res.success = true; // or false depending on your logic.
    return true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "pose_planner");
    ros::NodeHandle node_handle;

    PosePlanner pose_planner(node_handle);

    ros::Rate rate(500); // 500Hz

    while (ros::ok())
    {
        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
