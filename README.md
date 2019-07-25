# goFish | Finding Fish
An open source project for using stereo vision to Identify, Count, and Measure fish in the oceans.

---

## Getting Started
If you are setting up on a local machine, then skip to step 2.
1. Get setup on AWS, and create an EC2 instance (any distribution will do), and make sure it has enough storage and memory (minimum of 16GB and 8GB respectively should be fine). Then, SSH into the instance.
2. To get started with the project, clone this repo to your favourite project directory:
    ```bash
    cd ~/<project_directory>
    git clone https://github.com/cisco/goFish.git
    ```
    Once installed, run the [setup](#setup).

---

## Setup
#### Build
To setup the project, simply take a look at ```setup.sh```. If you are setting up on a new system, then you will need to run the full setup, or at least install OpenCV. Otherwise, you may can just build the project itself.

First, check the constants are correct (modify them as needed, though OpenCV version should NOT be lower than 4.0.0)
```bash
HOME_DIRECTORY=<home_directory>
PROJECT_DIRECTORY=~/<project_directory>
OPENCV_VERSION=4.1.0
```
Once you are satisfied with the above, then you'll need to run the setup.
- Full Installation: ```./setup.sh FULL``` (installs all dependencies [cmake, gcc, g++, golang-go], and opencv)
- Install OpenCV: ```./setup.sh OPENCV``` (installs the specified version of opencv)
- Regular Setup : ```./setup.sh```

#### Box
This application uses the Box API to backup files and reduce storage requirements on the server itself. Thus, to use the service, we need to setup an authentication process with our Box App.
1. Create a Box app at https://developer.box.com/.
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


---

## Run the Application
Once the project is setup, and all the executables have been created, all that's left is to run the main executable as ```./GoFish <port>```, where ```<port>``` is an open and exposed port. Igf left blank, it will simply run on port 80.

If this is being run on AWS, then run the program as ```./GoFish <port> &```, where ```<port>``` is the port number you exposed in the AWS security groups. The ```&``` will allow the program to run in the background, that way it stays running after you've exited out of the ssh session.

---

##Documentation
To generate the documentation for all the files, simply run ```./document.sh```. To vist=it the documentation, go to your web browser, and navigate to 
- C++: ```<your_domain>:<port>/docs/html```
- JS :```<your_domain>:<port>/docs/js```

There are different URLs for the different languages, as they are documented using different tools.
