import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, Command
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node


def generate_launch_description():

    use_sim_time = LaunchConfiguration('use_sim_time')
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    serial_device = LaunchConfiguration('serial_device')
    baud_rate = LaunchConfiguration('baud_rate')
    enc_counts_per_rev = LaunchConfiguration('enc_counts_per_rev')
    timeout_ms = LaunchConfiguration('timeout_ms')

    pkg_path = os.path.join(get_package_share_directory('diff_robot_description'))
    xacro_file = os.path.join(pkg_path, 'urdf', 'robot.xacro')
    robot_description_config = Command([
        'xacro ', xacro_file,
        ' sim_mode:=', use_sim_time,
        ' use_mock_hardware:=', use_mock_hardware,
        ' serial_device:=', serial_device,
        ' baud_rate:=', baud_rate,
        ' enc_counts_per_rev:=', enc_counts_per_rev,
        ' timeout_ms:=', timeout_ms
    ])

    params = {'robot_description': robot_description_config, 'use_sim_time': use_sim_time}
    node_robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[params]
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use sim time if true'
        ),
        DeclareLaunchArgument(
            'use_mock_hardware',
            default_value='false',
            description='Use mock hardware plugin if true'
        ),
        DeclareLaunchArgument(
            'serial_device',
            default_value='/dev/ttyUSB0',
            description='Serial port for motor controller'
        ),
        DeclareLaunchArgument(
            'baud_rate',
            default_value='115200',
            description='Baud rate for serial communication'
        ),
        DeclareLaunchArgument(
            'enc_counts_per_rev',
            default_value='340',
            description='Encoder counts per wheel revolution'
        ),
        DeclareLaunchArgument(
            'timeout_ms',
            default_value='1000',
            description='Serial communication timeout in milliseconds'
        ),
        node_robot_state_publisher
    ])
