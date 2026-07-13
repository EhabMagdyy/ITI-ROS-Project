from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():

    use_mock_hardware = LaunchConfiguration('use_mock_hardware')

    pkg = get_package_share_directory('diff_robot_bringup')

    display = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg, 'launch', 'display.launch.py')
        ),
        launch_arguments={'use_mock_hardware': use_mock_hardware}.items()
    )

    control = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg, 'launch', 'control.launch.py')
        )
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_mock_hardware',
            default_value='true',
            description='Use mock hardware plugin if true'
        ),
        display,
        control
    ])