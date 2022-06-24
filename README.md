# lonewolf
Repository for the bachelor thesis: ROS simulated world for ATV and finding optimal SLAM algorithm for generated point cloud

**If you are running this on NVIDIA AGX Xavier, head over to the .devcontainer for the setup guide.**

## Prerequisites
- ROS 2 Foxy installed on Ubuntu Linux 20.04 or newer
- You have installed Gazebo on your computer
- You have installed the required packages: 
  - https://github.com/RainerKuemmerle/g2o (NB! You have to download and build ceres-solver for g2o to work)
    - A tip when building g2o is to use ``` sudo make install```  instead off ```make``` 
 
    ```
    sudo apt install ros-foxy-gazebo-ros-pkgs
    sudo apt install ros-foxy-velodyne 
    sudo apt install ros-foxy-teleop-twist-keyboard
    sudo apt install ros-foxy-ros2-control
    sudo apt install ros-foxy-ros2-controllers
    sudo apt install ros-foxy-velodyne-gazebo-plugins
    ```
- You also need to install gtsam. 
  ```
  sudo add-apt-repository ppa:borglab/gtsam-release-4.0
  sudo apt update
  sudo apt install libgtsam-dev libgtsam-unstable-dev
  ```

Now you can download the simulator to your ros2 workspace. If you haven't set up a ros2 workspace before, you can follow [this tutorial](https://docs.ros.org/en/foxy/Tutorials/Workspace/Creating-A-Workspace.html).
```
source opt/ros/foxy/setup.bash
cd ~/ros2_ws/src
git clone git@github.com:sigridmellemseter/lonewolf.git
cd ~/ros2_ws
colcon build
```
Place the models in the folder gazebomodels inside the .gazebo/models/ folder. 



### Launching the simulator
Open a terminal and write the following. 
```
source opt/ros/foxy/setup.bash
cd ~/ros2_ws
source install/setup.bash
ros2 launch atv_pkg texas_world.launch.py
```
You should now see the ATV and the world in Gazebo. 

![Simulator in Gazebo](.images/gazebosim.png "Simulator")

### Steering the ATV

```
source opt/ros/foxy/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```
This application tells you how to steer the robot. 

### How view point cloud in rviz2: 
While the simulator is running, open a new terminal and run rviz2 
```
source opt/ros/foxy/setup.bash
rviz2 
```

In rviz2 you have to define the correct frame. Write `velodyne` as the Fixed Frame under Global Options. 

Now you have to add the PointCloud2 in rviz: 
- Click Add in the bottom left corner 
- Choose By Topic 
- Choose PointCloud2 under /points_raw

Now you should see the point cloud in rviz. To get a better visual choose the Style "Flat Squares", Size (m) to 0.03 and set Color Transformer to "AxisColor"

### Launching SLAM 
While the simulator is running, open a new terminal and write the following. 
```
source opt/ros/foxy/setup.bash
cd ~/ros2_ws
source install/setup.bash
ros2 launch scanmatcher lio_bigloop.launch.py
```
You can also view the maps in rviz2. 
- Open rviz2 as described above 
- Set the fixed frame to `map` 
- Add the topics `/map`, `/path`, `/modified_map` and `/modified_path`. 
  
To see the data more clearly, you can choose the following styles: 
- `/map` : Flat Squares, Size (m) to 0.03, Color Transformer to AxisColor
- `/path`: Color white
- `/modified_map`: Flat Squares, Size (m) 0.03, Color Transformer to ? 
- `/modified_path`: Color green

![Map, path, modified map and modified path i Rviz2](.images/SLAMcorrect.png "map from point cloud and SLAM")


