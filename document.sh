PROJECT_PATH=~

sudo spt-get install doxygen
sudo apg-tget install npm

cd $PROJECT_PATH/goFish/
doxygen
jsdoc static/scripts/*.js -d docs/js