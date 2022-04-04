# lonewolf
Repository for the bachelor thesis: ROS simulated world for ATV and finding optimal SLAM algorithm for generated point cloud

### How to open point cloud in rviz: 
Make sure you have the Velodyne package for foxy installed: 
```
sudo apt-get install ros-foxy-velodyne 
```
Then launch the simualor:
```
ros2 launch atv_pkg atv_world.launch.py
```
Open a new terminal and run rviz2 

```
ros2 run rviz2 rviz2 
```

In rviz2 you have to define the correct frame. Write `base_footprint` as the Fixed Frame under Global Options. 

Now you have to add the PointCloud2 in rviz: 
- Click Add in the bottom left corner 
- Choose By Topic 
- Choose PointCloud2 under /demo/gazebo_ros_laser_controller/out 

Now you should see the point cloud in rviz. To get a better visual choose the Style "Points" and set Color Transformer to "AxisColor"
