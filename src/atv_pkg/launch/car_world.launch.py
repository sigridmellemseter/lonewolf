# Copyright 2019 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Demo for spawn_entity.
Launches Gazebo and spawns a model
"""
# A bunch of software packages that are needed to launch ROS2
from http.server import executable
import os
from glob import glob
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.actions import ExecuteProcess
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import ThisLaunchFileDir,LaunchConfiguration
from launch_ros.actions import Node
import launch_ros.actions

#import xacro
#package_name = 'atv_pkg'

def generate_launch_description():

    use_sim_time = LaunchConfiguration('use_sim_time', default='True')
    world_file_name = 'new_forest.world'
    pkg_dir = get_package_share_directory('atv_pkg')

    os.environ["GAZEBO_MODEL_PATH"] = os.path.join(pkg_dir, 'models')

    world = os.path.join(pkg_dir, 'worlds', world_file_name)
    launch_file_dir = os.path.join(pkg_dir, 'launch')

    #unsure if this is needed or right
    #robot_description_path = os.path.join('share', package_name, 'models/atvkda/'), glob('./models/atvkda/*')
    
    #robot_description = {'robot_desription': xacro.process_file(robot_description_path).toxml} 

    #joint_state_publisher_node = Node(package='joint_state_publisher',
     #   executable='joint_state_publisher', name='joint_state_publisher')

    #robot_state_publisher_node = Node( package='robot_state_publisher',
        #executable='robot_state_publisher', output='both', parameters=[robot_description],)
    #end of unsureity

    gazebo = ExecuteProcess(
            cmd=['gazebo', '--verbose', world, '-s', 'libgazebo_ros_init.so', 
            '-s', 'libgazebo_ros_factory.so'],
            output='screen')

    #GAZEBO_MODEL_PATH has to be correctly set for Gazebo to be able to find the model
    #spawn_entity = Node(package='gazebo_ros', node_executable='spawn_entity.py',
    #                    arguments=['-entity', 'demo', 'x', 'y', 'z'],
    #                    output='screen')
    spawn_entity = Node(package='atv_pkg', executable='atv_node',
                        arguments=['atvkda', '', '0', '0', '1'],
                        output='screen')
    
   # forward_point_cloud_to_slam = Node(
   #     package='atv_pkg',
   #     executable='atv_node',
   #     name='atv_node',
   #     remappings=[
   #         ('/demo/gazebo_ros_laser_controller/out', '/velodyne_points'),
   #         ('/demo/imu_plugin/out', '/imu' ),
   #     ]
   # )


    return LaunchDescription([
        #joint_state_publisher_node, 
        #robot_state_publisher_node, 
        gazebo,
        spawn_entity,
    #    forward_point_cloud_to_slam,
    ])

    

