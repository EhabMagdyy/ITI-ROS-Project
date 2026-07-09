from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch.substitutions import Command
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    
    bringup_pkg = get_package_share_directory('diff_robot_bringup')
    desc_pkg = get_package_share_directory('diff_robot_description')
    
    xacro_file = os.path.join(desc_pkg, 'urdf', 'robot.xacro')
    
    rviz_config = os.path.join(bringup_pkg, 'config', 'display.rviz')
    

    robot_description = ParameterValue(
        Command(['xacro ', xacro_file]),
        value_type=str
    )

    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}]
        ),
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui'
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_config],
            
        ),
    ])