from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():

    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    serial_device = LaunchConfiguration('serial_device')
    baud_rate = LaunchConfiguration('baud_rate')
    enc_counts_per_rev = LaunchConfiguration('enc_counts_per_rev')
    timeout_ms = LaunchConfiguration('timeout_ms')

    pkg = get_package_share_directory('diff_robot_bringup')

    display = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg, 'launch', 'display.launch.py')
        ),
        launch_arguments={
            'use_mock_hardware': use_mock_hardware,
            'serial_device': serial_device,
            'baud_rate': baud_rate,
            'enc_counts_per_rev': enc_counts_per_rev,
            'timeout_ms': timeout_ms
        }.items()
    )

    control = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg, 'launch', 'control.launch.py')
        ),
        launch_arguments={
            'use_mock_hardware': use_mock_hardware,
            'serial_device': serial_device,
            'baud_rate': baud_rate,
            'enc_counts_per_rev': enc_counts_per_rev,
            'timeout_ms': timeout_ms
        }.items()
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_mock_hardware',
            default_value='true',
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
        display,
        control
    ])
