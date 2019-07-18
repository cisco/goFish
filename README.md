# goFish | Finding Fish
An open source project for using stereo vision to Identify, Count, and Measure fish in the oceans.

---

## Getting Started
If you are setting up on a local machine, then feel skip to step 2.
1. Get setup on AWS, and create an EC2 instance (any distribution will do), and make sure it has enough storage and memory (minimum of 16GB and 8GB respectively should be fine). Then, SSH into the instance.
2. To get started with the project, clone this repo to your favourite project directory:
    ```bash
    cd ~/<project_directory>
    git clone https://github.com/cisco/goFish.git
    ```
    Once installed, run the [setup](#setup).

---

## Setup
To setup the project, simply take a look at ```setup.sh```, and check that all you require verything in there. For instance, if you are setting up on a new system, then you will need to run the full setup. Otherwise, you may just need to build the project itself.

First, check the constants are correct (modify them as needed, though OpenCV version should NOT be lower than 4.0.0)
```bash
HOME_DIRECTORY=/home/ubuntu
PROJECT_DIRECTORY=~
OPENCV_VERSION=4.1.0
```
Once you are satisfied with the above, then you'll need to run the setup.
- Full Installation: ```./setup.sh FULL``` (installs all dependencies [cmake, gcc, g++, golang-go], and opencv)
- Install OpenCV: ```./setup.sh OPENCV``` (installs the specified version of opencv)
- Regular Setup : ```./setup.sh```

---

## Run the Application
Once the project is setup, and all the executables have been created, all that's left is to run the main executable as ```./GoFish <port>```, where ```<port>``` is an open and exposed port. Igf left blank, it will simply run on port 80.

If this is being run on AWS, then run the program as ```./GoFish <port> &```, where ```<port>``` is the port number you exposed in the AWS security groups. The ```&``` will allow the program to run in the background, that way it stays running after you've exited out of the ssh session.
