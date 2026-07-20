import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    serial_port = LaunchConfiguration('serial_port')
    frame_id = LaunchConfiguration('frame_id')

    return LaunchDescription([
        DeclareLaunchArgument(
            'serial_port',
            default_value='/dev/ttyUSB0',
            description='Serial port for RPLidar'),
        DeclareLaunchArgument(
            'frame_id',
            default_value='lidar_link',
            description='Frame ID for the laser scan'),

        Node(
            package='rplidar_ros',
            executable='rplidar_composition',
            output='screen',
            parameters=[{
                'serial_port': serial_port,
                'serial_baudrate': 115200,
                'frame_id': frame_id,
                'angle_compensate': True,
                'scan_mode': 'Sensitivity',
                'auto_standby': True
            }]
        )
    ])
