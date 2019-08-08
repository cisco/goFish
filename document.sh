#!/bin/bash
PROJECT_PATH=~

apt-get -y install doxygen
apt-get -y install npm
npm install -g jsdoc

cd /goFish/
doxygen
jsdoc /goFish/static/scripts/*.js -d docs/js