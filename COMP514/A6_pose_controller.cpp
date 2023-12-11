#include <pinocchio/multibody/model.hpp>
#include <pinocchio/multibody/data.hpp>
#include <pinocchio/parsers/urdf.hpp>
#include <pinocchio/algorithm/kinematics.hpp>
#include <pinocchio/algorithm/jacobian.hpp>
#include <pinocchio/algorithm/joint-configuration.hpp>
#include <pinocchio/algorithm/compute-all-terms.hpp>
#include <pinocchio/math/quaternion.hpp>
#include <pinocchio/math/rpy.hpp>
#include <ros/ros.h>
#include <Eigen/Dense>
#include <tf/tf.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Quaternion.h>
#include "highlevel_msgs/MoveTo.h"
#include <std_msgs/Float64MultiArray.h>
#include <sensor_msgs/JointState.h>

class Planner
{
public:
    int loop_rate;
    Planner(ros::NodeHandle nh)
    {
        this->nh = nh;
        translation = nh.advertiseService("/pose_planner/move_to", &Planner::moveToCallback, this);
        orientation = nh.advertiseService("/pose_planner/move_ori", &Planner::moveOriCallback, this);
        sub = nh.subscribe("/gen3/joint_states", 1, &Planner::jointCallback, this);
        pub = nh.advertise<std_msgs::Float64MultiArray>("/gen3/joint_group_position_controller/command", 1);

        initParams();
    }

private:
    // ROS variables
    ros::NodeHandle nh;
    ros::ServiceServer translation, orientation;
    ros::ServiceClient client;
    ros::Subscriber sub;
    ros::Publisher pub;

    std::string filename;
    std_msgs::Float64MultiArray pub_msg;

    pinocchio::Model model;
    pinocchio::Data data;

    // joint states
    Eigen::VectorXd current_q = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd end_q = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd vel = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd vel_red = Eigen::VectorXd::Zero(7);

    // joint limits
    Eigen::VectorXd min_q = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd min_vel = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd max_q = Eigen::VectorXd::Zero(7);
    Eigen::VectorXd max_vel = Eigen::VectorXd::Zero(7);

    // jacobians
    Eigen::MatrixXd J = Eigen::MatrixXd::Zero(6, 7);
    Eigen::MatrixXd J_pinv = Eigen::MatrixXd::Zero(7, 6);    // pseudo-inverse of jacobian
    Eigen::MatrixXd null_proj = Eigen::MatrixXd::Zero(7, 7); // null space projection matrix
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(7, 7);

    // double random_constant;                     // magical random constant
    double current_T = 0;                       // Current time
    double a2, a3 = 0;                          // Coefficients of the polynomial
    double x_0, y_0, z_0, T_0 = 0;              // Initial position and time
    double end_x, end_y, end_z, end_T = 0;      // Target position and time
    double end_roll, end_pitch, end_yaw = 0;    // Target orientation
    double roll_init, pitch_init, yaw_init = 0; // Initial orientation

    Eigen::Quaterniond ori_init = Eigen::Quaterniond(0, 0, 0, 1);
    Eigen::Quaterniond current_ori = Eigen::Quaterniond(0, 0, 0, 1);
    Eigen::Quaterniond end_ori = Eigen::Quaterniond(0, 0, 0, 1);
    Eigen::Quaterniond ori_next = Eigen::Quaterniond(0, 0, 0, 1);
    pinocchio::SE3 pose_ref;
    pinocchio::SE3 pose_error;
    Eigen::VectorXd pose_error_world = Eigen::VectorXd::Zero(6);
    Eigen::VectorXd pose_error_vec = Eigen::VectorXd::Zero(6);

    Eigen::VectorXd x_dot = Eigen::VectorXd::Zero(6);
    bool moving = false;
    int moveto = 0;
    int counter = 0;
    bool rotating = false;

    bool moveToCallback(highlevel_msgs::MoveTo::Request &req, highlevel_msgs::MoveTo::Response &res);
    bool moveOriCallback(highlevel_msgs::MoveTo::Request &req, highlevel_msgs::MoveTo::Response &res);
    void jointCallback(const sensor_msgs::JointState::ConstPtr &msg);
    void sendJoints();
    void computeCoefficients();
    void initParams();
    void move();
    void get_J_pinv();
};

// function to run to initialize parameters after geend_Ting a service request
void Planner::initParams()
{
    // Temp recipients of params
    std::vector<double> end_pos_;
    std::vector<double> end_ori_;

    // Get params from parameter server
    nh.getParam("/publish_rate", loop_rate);
    nh.getParam("/gen3/urdf_file_name", filename);
    nh.getParam("/gen3/target/hand/position", end_pos_);
    nh.getParam("/gen3/target/hand/orientation", end_ori_);

    // Convert to Eigen vectors
    end_x = end_pos_[0];
    end_y = end_pos_[1];
    end_z = end_pos_[2];
    end_T = 4; // Default value

    // Convert to Eigen quaternions
    pinocchio::quaternion::assignQuaternion(end_ori, pinocchio::rpy::rpyToMatrix(end_ori_[0], end_ori_[1], end_ori_[2]));

    pinocchio::urdf::buildModel(filename, model, false);
    data = pinocchio::Data(model);

    // Set initial params from tutorial
    min_q << -10.0, -2.41, -10.0, -2.66, -10.0, -2.23, -10.0;
    min_vel << -1.3963, -1.3963, -1.3963, -1.3963, -1.2218, -1.2218, -1.2218;
    max_q << 10.0, 2.41, 10.0, 2.66, 10.0, 2.23, 10.0;
    max_vel << 1.3963, 1.3963, 1.3963, 1.3963, 1.2218, 1.2218, 1.2218;

    // Set joint limits in model
    for (int i = 0; i < 7; i++)
    {
        model.lowerPositionLimit[i] = min_q(i);
        model.upperPositionLimit[i] = max_q(i);
    }

    return;
}

// function to run at the first instance of movement after the node is started
void Planner::move()
{
    // Save current end effector position
    x_0 = data.oMi[7].translation()(0);
    y_0 = data.oMi[7].translation()(1);
    z_0 = data.oMi[7].translation()(2);
    T_0 = ros::Time::now().toSec();
    // random_constant = 4;

    computeCoefficients();
    moving = true;
    moveto = 1;

    // publish current joint states
    pub_msg.data.resize(7);
    for (int i = 0; i < 7; i++)
    {
        pub_msg.data[i] = current_q(i);
    }
    pub.publish(pub_msg);
    return;
}

// function to run every time a joint state is received
void Planner::jointCallback(const sensor_msgs::JointState::ConstPtr &msg)
{
    // Get current joint states and velocities from message if we are already moving
    if (!moving)
    {
        for (int i = 0; i < 7; i++)
        {
            current_q(i) = msg->position[i];
            vel(i) = msg->velocity[i];
        }
    }

    pinocchio::forwardKinematics(model, data, current_q, vel); // Forward kinematics to update data

    current_ori = Eigen::Quaterniond(data.oMi[7].rotation()); // get current orientation
    current_T = ros::Time::now().toSec();                     // Get current time

    if (moving) // Check if we are moving
    {
        sendJoints();
    }
    if (counter == 0) // run move() at the first instance of movement
    {
        move();
        counter++;
    }
}

// function to run when a service request is received to move to a position
bool Planner::moveToCallback(highlevel_msgs::MoveTo::Request &req, highlevel_msgs::MoveTo::Response &res)
{
    // Save current end effector position
    x_0 = data.oMi[7].translation()(0);
    y_0 = data.oMi[7].translation()(1);
    z_0 = data.oMi[7].translation()(2);
    T_0 = ros::Time::now().toSec();
    // random_constant = 4;

    // Save target end effector position
    end_x = req.x;
    end_y = req.y;
    end_z = req.z;
    end_T = req.T;
    end_T = end_T / 1.25; // Divide by 1.25 to account for the fact that the robot moves slower than the target speed

    if (end_T <= 0 || end_z <= 0) // Check if the target time and position is valid
    {
        res.success = false;
        return false;
    }
    res.success = true; // If valid, set success to true

    computeCoefficients();
    moving = true;
    moveto = 1;

    // reinitialize pub_msg for publishing
    pub_msg.data.resize(7);

    for (int i = 0; i < 7; i++)
    {
        pub_msg.data[i] = current_q(i);
    }
    pub.publish(pub_msg);
    return true;
}

// function to run when a service request is received to move to an orientation
bool Planner::moveOriCallback(highlevel_msgs::MoveTo::Request &req, highlevel_msgs::MoveTo::Response &res)
{
    // Save the start position of the movement, we get the position of the 7th joint since that is the end-effector for this robot
    x_0 = data.oMi[7].translation()(0);
    y_0 = data.oMi[7].translation()(1);
    z_0 = data.oMi[7].translation()(2);
    T_0 = ros::Time::now().toSec();
    ori_init = Eigen::Quaterniond(data.oMi[7].rotation());
    // random_constant = 4;

    // Save target end effector orientation
    end_roll = req.x;
    end_pitch = req.y;
    end_yaw = req.z;

    // rotating = true;

    pinocchio::quaternion::assignQuaternion(end_ori, pinocchio::rpy::rpyToMatrix(end_roll, end_pitch, end_yaw)); // Convert to Eigen quaternions
    end_T = req.T / 1.25;                                                                                        // Divide by 1.25 to account for the fact that the robot moves slower than the target speed

    if (end_T <= 0) // Check if the target time is valid
    {
        res.success = false;
        return false;
    }
    res.success = true;
    computeCoefficients();
    moving = true;

    // reinitialize pub_msg for publishing
    pub_msg.data.resize(7);
    for (int i = 0; i < 7; i++)
    {
        pub_msg.data[i] = current_q(i);
    }
    pub.publish(pub_msg);
    return true;
}

void Planner::computeCoefficients()
{
    // Compute non zero parts of the polynomial
    a2 = 3 / (end_T * end_T);
    a3 = -2 / (end_T * end_T * end_T);
}

void Planner::get_J_pinv()
{
    pinocchio::computeAllTerms(model, data, current_q, vel);                                        // Computes the generalized velocities / accelerations of all the joints, given current state and updates the jacobian accordingly
    pinocchio::getJointJacobian(model, data, 7, pinocchio::ReferenceFrame::LOCAL_WORLD_ALIGNED, J); // Get jacobian of end effector
    J_pinv = J.completeOrthogonalDecomposition().pseudoInverse();                                   // Get pseudo-inverse of the Jacobian to task space to joint space
}

void Planner::sendJoints()
{
    double dt = current_T - T_0; // get delta
    // double s = 3 * (dt * dt) / (end_T * end_T) - 2 * (dt * dt * dt) / (end_T * end_T * end_T);
    double poly = (a2 * dt * dt + a3 * dt * dt * dt); // get polynomial
    ori_next = ori_init.slerp(poly, end_ori);         // get next orientation

    // Compute new end effector position
    pose_ref.translation()(0) = x_0 + poly * (end_x - x_0);
    pose_ref.translation()(1) = y_0 + poly * (end_y - y_0);
    pose_ref.translation()(2) = z_0 + poly * (end_z - z_0);
    pose_ref.rotation() = ori_next.toRotationMatrix(); // convert to rotation matrix and set as target orientation

    // Compute pose error and convert to vector
    pose_error = data.oMi[7].inverse() * pose_ref;
    pose_error_vec = pinocchio::log6(pose_error).toVector();
    pose_error_world.head(3) = pose_ref.translation() - data.oMi[7].translation();
    pose_error_world.tail(3) = data.oMi[7].rotation() * pose_error_vec.tail(3);

    // Compute pose error and convert to vector
    // pose_error = data.oMi[7].inverse() * pose_ref;
    // pose_error_vec = pinocchio::log6(pose_error).toVector();

    // // If we are not moving publish current joint states
    // if (moveto == 0 && !rotating)
    // {
    //     vel = Eigen::VectorXd::Zero(7);
    //     vel_red = Eigen::VectorXd::Zero(7);
    //     for (int i = 0; i < 7; i++)
    //     {
    //         pub_msg.data[i] = current_q(i);
    //     }
    //     pub.publish(pub_msg);
    //     return;
    // }

    if (current_T - T_0 >= end_T) // check if we have reached the target time
    {
        moveto = 0; // set moveto to 0

        // set translation to target end effector position
        pose_ref.translation()(0) = end_x;
        pose_ref.translation()(1) = end_y;
        pose_ref.translation()(2) = end_z;
        pose_ref.rotation() = end_ori.toRotationMatrix(); // convert to rotation matrix

        // Compute pose error and convert to vector
        pose_error = data.oMi[7].inverse() * pose_ref;
        pose_error_vec = pinocchio::log6(pose_error).toVector();

        // Compute pose error and convert to vector
        pose_error_world.head(3) = pose_ref.translation() - data.oMi[7].translation();
        pose_error_world.tail(3) = data.oMi[7].rotation() * pose_error_vec.tail(3);

        // Avert your eyes from this section if you enjoy clean implementations

        // // check how close we are of the target position
        // double x_error = abs(end_x - data.oMi[7].translation()(0));
        // double y_error = abs(end_y - data.oMi[7].translation()(1));
        // double z_error = abs(end_z - data.oMi[7].translation()(2));

        // if (x_error < 0.05 && y_error < 0.05 && z_error < 0.05) // I cry
        // {
        //     random_constant = 2;
        //     if (x_error < 0.01 && y_error < 0.01 && z_error < 0.01) // if we are within 1 cm of the target position
        //     {
        //         moveto = 0; // set moving to false
        //         moving = false;
        //     }
        // }

        // double yaw_error = abs(end_roll - data.oMi[7].rotation().eulerAngles(0, 1, 2)(0));
        // double pitch_error = abs(end_pitch - data.oMi[7].rotation().eulerAngles(0, 1, 2)(1));
        // double roll_error = abs(end_yaw - data.oMi[7].rotation().eulerAngles(0, 1, 2)(2));

        // if (yaw_error < 0.05 && pitch_error < 0.05 && roll_error < 0.05) // if we are within 1 cm of the target position
        // {
        //     random_constant = 2;
        //     if (yaw_error < 0.01 && pitch_error < 0.01 && roll_error < 0.01) // if we are within 1 cm of the target position
        //     {
        //         rotating = false;
        //         moving = false;
        //     }
        // }
    }

    get_J_pinv();                    // get pseudo inverse of jacobian
    null_proj = I - J_pinv * J;      // get null space projection matrix
    vel = J_pinv * pose_error_world; // get joint velocities from jacobian and pose error world
    vel_red = null_proj * vel;       // project joint velocities onto null space
    vel += vel_red;                  // add null space velocities to joint velocities

    for (int i = 0; i < 7; i++) // Check if joint velocities are within limits
    {
        if (vel(i) < min_vel(i))
        {
            vel(i) = min_vel(i);
        }
        else if (vel(i) > max_vel(i))
        {
            vel(i) = max_vel(i);
        }
    }

    current_q = current_q + 16 * (vel / loop_rate); // update current joint states

    // publish current joint states
    pub_msg.data.resize(7);
    for (int i = 0; i < 7; i++)
    {
        pub_msg.data[i] = current_q(i);
    }
    pub.publish(pub_msg);
}

int main(int argc, char **argv)
{

    ros::init(argc, argv, "planner_node");

    ros::NodeHandle nh;

    ROS_INFO("Creating Planner");
    Planner planner(nh);

    ros::Rate loop_rate(planner.loop_rate);

    while (ros::ok())
    {
        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}