import os
from launch import LaunchDescription
from launch_ros.actions import Node
 
 
def generate_launch_description():
 
  return LaunchDescription([
    Node(package='atv_controller_pkg', executable='atv_controller',
      output='screen'),
    Node(package='atv_controller_pkg', executable='atv_estimator',
      output='screen'),
  ])