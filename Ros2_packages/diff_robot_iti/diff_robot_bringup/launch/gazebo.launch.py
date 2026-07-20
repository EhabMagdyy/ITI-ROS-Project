import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction, RegisterEventHandler
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.event_handlers import OnProcessStart
from launch_ros.actions import Node


def generate_launch_description():

    pkg_bringup = get_package_share_directory('diff_robot_bringup')

    rsp = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            pkg_bringup,
            'launch',
            'rsp.launch.py'
        )]),
        launch_arguments={'use_sim_time': 'true', 'use_mock_hardware': 'false'}.items()
    )

    gazebo_params_file = os.path.join(
        pkg_bringup,
        'config',
        'gazebo_params.yaml'
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory('gazebo_ros'),
            'launch',
            'gazebo.launch.py'
        )]),
        launch_arguments={
            'verbose': 'true',
            'extra_gazebo_args': '--ros-args --params-file ' + gazebo_params_file
        }.items()
    )

    spawn_entity = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=['-topic', 'robot_description',
                   '-entity', 'my_robot',
                   '-z', '0.1'],
        output='screen'
    )

    controller_params_file = os.path.join(
        pkg_bringup,
        'config',
        'controllers.yaml'
    )

    controller_manager = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {'use_sim_time': True},
            controller_params_file
        ],
        output='screen'
    )

    delayed_controller_manager = TimerAction(
        period=5.0,
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

    delayed_spawners = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[joint_state_broadcaster_spawner, diff_drive_spawner],
        )
    )

    # EKF for Gazebo (uses Gazebo IMU topic)
    gazebo_ekf_config_file = os.path.join(
        pkg_bringup,
        'config',
        'gazebo_ekf.yaml'
    )

    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[gazebo_ekf_config_file],
        remappings=[('/odometry/filtered', '/odom')]
    )

    return LaunchDescription([
        rsp,
        gazebo,
        spawn_entity,
        delayed_controller_manager,
        delayed_spawners,
        ekf_node
    ])
