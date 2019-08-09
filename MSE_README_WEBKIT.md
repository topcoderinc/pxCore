# Spark - MSE Webkit port

tested on ubuntu 18.04 LTS

# Build steps:

apt update

apt install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad 
apt install gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x 
apt install gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio

cd examples/pxScene2d/external/
bash build.sh
cd ../../..
mkdir temp
cd temp
cmake ..
make

cd ../examples/pxScene2d/src

./Spark mse_demo.js

// check mjpeg.avi
