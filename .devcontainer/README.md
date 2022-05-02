# Docker
The DockerFiles in this project is intended for being able to run ROS2 Foxy on the NVIDIA AGX Xavier. The AGX runs on JetPack, that currently only supports Ubuntu 18.04. When JetPack 5 is released later in 2022, it will not be necessary to run ROS2 through Docker. 

## Installation
  ```
  sudo apt update
  ```
Install Docker
- [Ubuntu](https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-18-04)
- [macOS](https://docs.docker.com/desktop/mac/install/)
- [Windows](https://www.youtube.com/watch?v=dQw4w9WgXcQ)

Note:

If you are using Ubuntu, remember to add docker to the admin group so you don't have to add sudo 
before every docker command: https://docs.docker.com/engine/install/linux-postinstall/ . After doing 
this, remember to log out and back in (or just reboot), because the change only takes effect after doing this.


## Setting up development with VSCode
If you are new to ubuntu/linux, or don't already have any other specific plan on how you want to set up 
development with docker, we recommend you do this.
1. Download the repo with the command in your terminal
  ```
  git clone git@github.com:sigridmellemseter/lonewolf.git
   ```
2. Install Visual Studio Code on your host computer. Then download the `Remote - Containers`-extension
3. To open the container in VSCode do `CTRL+SHIFT+P` and search for `Remote-Containers: Reopen in Container`.
he image should now start building. This might take a while.
4. Open a terminal in VSCode and install and build Ceres Solver with these commands:
 ```
 git clone https://ceres-solver.googlesource.com/ceres-solver
 cd ceres-solver
 mkdir ceres-bin
 cd ceres-bin
 cmake ../
 make -j3
 make test
 make install
   ```
5. Install and build g2o with this commands in the lonewolf directory :
 ```
  git clone git@github.com:RainerKuemmerle/g2o.git
  cd g20
  mkdir build
  cd build
  cmake ../
  make
   ```
6. You can now start developing by building the pipeline by using `colcon build` 



## Docker container FAQ
- **Q:** What does it mean to be *inside* the docker container? **A:** When you open the terminal, you will see a
 signature next to your cursor: `<your_username>@<your_computer_name>:~$`.ou are in the command line locally on
 your computer when this username is the one you set and the computer name is also the one you set.
 
- **Q:** How to enter the Docker container? **A:** You can enter the Docker container by using the command `code lonewolf`. When you are in VSCode, you have to open the container by doing `CTRL+SHIFT+P` and search for `Remote-Containers: Reopen in Container`. The image should now start building. After the first time you have done this, you should get a notification in the bottom right corner of VSCode that asks you if you want to open the workspace in a Remote Container. After the image is built, open a terminal in VSCode and you are inside the container. 
- **Q:** What is the DockerFile based on? **A:** The DockerFile is based on the `althack/ros2:foxy-gazebo-nvidia` Docker image, but has added dependencies necessary for running SLAM. 
