import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import TimerAction, RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.substitutions import LaunchConfiguration, Command
from launch.actions import DeclareLaunchArgument

from launch_ros.actions import Node


def generate_launch_description():

    package_name = 'diff_robot_bringup'
    desc_pkg = 'diff_robot_description'

    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    serial_device = LaunchConfiguration('serial_device')
    baud_rate = LaunchConfiguration('baud_rate')
    enc_counts_per_rev = LaunchConfiguration('enc_counts_per_rev')
    timeout_ms = LaunchConfiguration('timeout_ms')

    pkg_path = os.path.join(get_package_share_directory(desc_pkg))
    xacro_file = os.path.join(pkg_path, 'urdf', 'robot.xacro')
    robot_description = Command([
        'xacro ', xacro_file,
        ' sim_mode:=false',
        ' use_mock_hardware:=', use_mock_hardware,
        ' serial_device:=', serial_device,
        ' baud_rate:=', baud_rate,
        ' enc_counts_per_rev:=', enc_counts_per_rev,
        ' timeout_ms:=', timeout_ms
    ])

    controller_params_file = os.path.join(
        get_package_share_directory(package_name),
        'config',
        'controllers.yaml'
    )

    controller_manager = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {'robot_description': robot_description},
            controller_params_file
        ],
        output='screen'
    )

    delayed_controller_manager = TimerAction(
        period=3.0,
        actions=[controller_manager]
    )

    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
        output='screen'
    )

    diff_drive_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['diff_cont'],
        output='screen'
    )

    imu_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['imu_broadcaster'],
        output='screen'
    )

    delayed_spawners = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[joint_state_broadcaster_spawner, diff_drive_spawner, imu_broadcaster_spawner],
        )
    )

    ekf_config_file = os.path.join(
        get_package_share_directory(package_name),
        'config',
        'ekf.yaml'
    )

    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config_file],
        remappings=[('/odometry/filtered', '/odom')]
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
        delayed_controller_manager,
        delayed_spawners,
        ekf_node
    ])
