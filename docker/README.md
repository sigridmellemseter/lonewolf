# Docker

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
1. Install Visual Studio Code on your host computer. Then download the `Remote - Containers`-extension
2. Script to build the file???
3. To open the container in VSCode do `CTRL+SHIFT+P` and search for `Remote-Containers: Reopen in Container`.
The image should now start building.
4. Once inside the container you can start by building the pipeline. Use `CTRL+SHIFT+B`, or enter colcon build (???)



## Docker container FAQ
- **Q:** What does it mean to be *inside* the docker container? **A:** When you open the terminal, you will see a
 signature next to your cursor: `<your_username>@<your_computer_name>:~$`.ou are in the command line locally on
 your computer when this username is the one you set and the computer name is also the one you set. You are
 inside the container if the user name is `tbd`, and the computer name is a bunch of letters and numbers.
 
- **Q:** How to enter the Docker container? **A:** 
