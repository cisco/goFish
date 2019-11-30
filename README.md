# goFish | Finding Fish
[![license](https://img.shields.io/badge/license-BSD%202--Clause-blue)](https://github.com/cisco/goFish/blob/master/LICENSE)
[![Build Status](https://travis-ci.com/cisco/goFish.svg)](https://travis-ci.com/cisco/goFish) [![CircleCI](https://circleci.com/gh/cisco/goFish.svg?style=svg)](https://circleci.com/gh/cisco/goFish)

An open source project for using stereo vision to Identify, Count, and Measure fish in the oceans.

[![dockerhub](https://img.shields.io/badge/dockerhub-ghostofcookie%2Fgofish-informational?style=for-the-badge&logo=docker)](https://hub.docker.com/r/ghostofcookie/gofish)

---

## Getting Started
If you are setting up on a local machine, then skip to step 2.
1. Get setup on AWS, and create an EC2 Ubuntu instance, and make sure it has enough storage and memory (minimum of 16GB and 8GB respectively should be fine). Then, SSH into the instance.
2. To get started with the project, clone this repo to your favourite project directory:
    ```bash
    cd ~/<project_directory>
    git clone https://github.com/cisco/goFish.git
    ```
    Once installed, run the [setup](#setup).

---

## Setup
There are 2 different workflows for this project. It can be setup, built, and tested using either [Docker](#docker), or else it can all be handled [manually](#manual).

No matter which method you choose, there are still some requirements that need to be met before the project will run properly:
- Setup an app with the [Box](#box) API, and authorize it in the [Box Admin Console](https://app.box.com/master).

### Required

#### Box
This application uses the Box API to backup files and reduce storage requirements on the server itself. Thus, to use the service, we need to setup an authentication process with our Box App.
1. Create a Box app at https://developer.box.com/ (**DON'T FORGET TO AUTHORIZE THE APP IN THE ADMIN CONSOLE**).
2. Open up your newly created Box app, and go to **Configuration**. Here, you need to change **Application Access** from "Application" to "Enterprise".
3. Generate private and public RSA keys using openssl:
    ```bash
    openssl genrsa -des3 -out private.pem 2048
    openssl rsa -in private.pem -outform PEM -pubout -out public.pem
    ```
    *Save these somewhere safe! 
4. Now that you have the private and public keys, import the public key into your Box App in the **Configuration** section. Once you have that, you need to do one of two things:
    1. Download the **App Settings** as JSON at the bottom of the **Configuration** section in your Box developer App, then simply copy the contents of your private.pem file into the private key section. OR,
    2. In the project directory, navigate to config/ and copy the "box_jwt_template.json" file, then make a directory in the project root called "private_config", and save the copy of the template in there as "box_jwt.json", and fill out all the information using your public and private keys, as well as any other info that you can aquire from your Box Developer App **Configuration** section.

    The template is also included here just in case:
    ```json
    {
        "boxAppSettings": {
            "clientID": "",
            "clientSecret": "",
            "appAuth": {
                "publicKeyID": "<public_key_ID>",
                "privateKey": "<private_key>",
                "passphrase": ""
            }
        },
        "enterpriseID": ""
    }
    ```
    No matter which option you chose, save the settings file in "private_config" as "box_jwt.json". You need to create the folder "private_config" in the root of the project folder (i.e. ~/<project_directory>/goFish/private_config).


### Docker
The Docker image is already built and saved in [DockerHub](https://hub.docker.com/r/ghostofcookie/gofish), which means that there is no need to worry about dependencies, as they have already been installed inside the image. All that is left is to build and test the project.

To build the Docker image, first pull the latest version
```bash
docker pull ghostofcookie/gofish:latest
```

Then, navigate to the project root directory, then build the image
```bash
docker build -t ghostofcookie/gofish:<tag> .
```

To build the project inside of Docker, simply run
```bash
docker run ghostofcookie/gofish:<tag> /goFish/build.sh
```

To run tests for Go and C++, use
```bash
docker run ghostofcookie/gofish:<tag> /goFish/run_tests.sh
```

### Manual
If you do not wish to use the docker setup, then the following section covers how to get setup with all of the appropriate dependencies, as well as how to build and test the whole project.

#### Dependencies
To set up the project, simply take a look at ```setup.sh```. If you are setting up on a new system, then you will need to run the full setup, or at least install OpenCV. Otherwise, you can just build the project itself.

To setup the project with dependencies, run one of the following:
- Full Installation: ```./setup.sh FULL``` (installs all dependencies [cmake, gcc, g++, golang-go], and opencv)
- Install OpenCV: ```./setup.sh OPENCV``` (installs the specified version of opencv)
- Regular Setup : ```./setup.sh```

#### Build
First, make sure you have all dependencies installed (see [above](#dependencies)). To build the entire project, use the ```build.sh``` script, by simply running

```bash
./build.sh
```

To build only the Go files, navigate to the ```goServer/``` directory, and run ```go build```.

To build only the C++ files, navigate to ```findFish/```, and run 
```bash
mkdir -p build
cd build
cmake .. && make
```
This will compile the entire project (Go and C++ alike).

To run tests for the entire project, simply run

```bash
./run_tests.sh
```

To test only the Go files, navigate to the ```goServer/``` directory, and run ```go test```.

To test only the C++ files, navigate to ```tests/```, and run 
```bash
mkdir -p build
cd build
cmake .. && make
./run_tests
```

---

## Run the Application
Once the project is setup, and all the executables have been created, all that's left is to run the main executable as ```./GoFish <port>```, where ```<port>``` is an open and exposed port. If left blank, it will simply run on port 80.

If this is being run on AWS, then run the program as ```./GoFish <port> &```, where ```<port>``` is the port number you exposed in the AWS security groups. The ```&``` will allow the program to run in the background, that way it stays running after you've exited out of the ssh session.

---

## Documentation
To generate the documentation for all the files, simply run ```./document.sh```. To visit the documentation, go to your web browser, and navigate to 
- C++: ```<your_domain>:<port>/static/docs/html```
- JS :```<your_domain>:<port>/static/docs/js```

There are different URLs for the different languages, as they are documented using different tools.
