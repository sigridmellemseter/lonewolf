#!/bin/bash
if [ ! -z $GAZEBO_MASTER_URI ]; then
   tmp_GAZEBO_MASTER_URI=$GAZEBO_MASTER_URI
fi

cd /home/robomaker/workspace/aws-robomaker-sample-application-helloworld-ros2/simulation_ws
source /opt/ros/foxy/setup.bash
source /usr/share/gazebo-11/setup.sh

 if [ ! -z $tmp_GAZEBO_MASTER_URI ]; then
    export GAZEBO_MASTER_URI=$tmp_GAZEBO_MASTER_URI
    unset tmp_GAZEBO_MASTER_URI 
fi

source ./install/setup.sh
printenv

exec "${@:1}"