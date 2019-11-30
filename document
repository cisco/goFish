#!/bin/bash

apt-get -y install doxygen
apt-get -y install npm
npm install -g jsdoc

cd /goFish

mkdir docs/
ln -sfn docs/ static/
cd findFish/ || exit
doxygen
cd .. || exit
jsdoc static/scripts/*.js -d docs/js