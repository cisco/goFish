PROJECT_PATH=~

sudo apt-get install doxygen
sudo apt-get install npm
npm install -g jsdoc

cd $PROJECT_PATH/goFish/
doxygen
jsdoc static/scripts/*.js -d docs/js