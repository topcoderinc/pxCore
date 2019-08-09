# Spark - MSE Webkit port

port of examples/pxScene2d/src/mse_demo.js using mse webkit pipeline.
the result is written to mjpeg.avi in current directory.


tested on ubuntu 18.04 LTS 64 bit
g++ (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0

# Build steps:

apt update

apt install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad 
apt install gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x 
apt install gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
apt install jhbuild

cd examples/pxScene2d/external/
bash build.sh

// the webkit is failing on compiling ../../Source/WebKit/UIProcess/API/glib/WebKitNavigationPolicyDecision.cpp
// it's ok because all needed libraries are already built

cd ../../..
mkdir temp
cd temp
cmake ..
make

cd ../examples/pxScene2d/src

./Spark mse_demo.js

// check mjpeg.avi


# Troubleshooting:

- run with envvar GST_DEBUG="*:2" to troubleshoot gstreamer
- ensure gstreamer correctly locates plugins (gst-play-1.0 <some-video-file>)
