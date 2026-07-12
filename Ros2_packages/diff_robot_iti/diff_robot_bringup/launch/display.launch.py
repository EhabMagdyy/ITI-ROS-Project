import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():

    # Include rsp.launch.py with sim_time=false
    rsp = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory('diff_robot_bringup'),
            'launch',
            'rsp.launch.py'
        )]), launch_arguments={'use_sim_time': 'false'}.items()
    )

    rviz_config = os.path.join(
        get_package_share_directory('diff_robot_bringup'),
        'config',
        'display.rviz'
    )

    return LaunchDescription([
        rsp,
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui'
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_config]
        ),
    ])