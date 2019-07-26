PROJECT_PATH=~

sudo apt-get -y install doxygen
sudo apt-get -y install npm
sudo npm install -g jsdoc

cd $PROJECT_PATH/goFish/
doxygen
jsdoc static/scripts/*.js -d docs/js