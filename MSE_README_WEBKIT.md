# Spark - MSE Webkit port  
  
# Build steps ubuntu (tested on ubuntu 18.04 LTS  64 bit):  
  
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

# Build steps macos (tested on highsierra and mojave)  
1) install xcode 9.4  
2) xcode-select --install  
3) install homebrew (brew.sh)  
4) install macports (macports.org)  
5) sudo port install webkit-gtk xorg-server webkit-gtk3  
6) sudo port install gtk3 quilt cmake nodejs12  
7) sudo port install wget pkgconfig openssl libffi libuv npm6  
8) brew install openssl libffi libuv  
   brew cask install java  
sudo chmod -R $(whoami) /opt/local

reboot  

9) cd examples/pxScene2d/external/  
bash build.sh    
10)   
cd ../../..   
mkdir temp    
cd temp    
cmake -DCMAKE_EXE_LINKER_FLAGS=" -L /opt/local/lib " ..    
make  

# Testing
  
cd ../examples/pxScene2d/src  

./Spark mse_demo_video_only.js  

./Spark mse_demo_video_audio.js   

./Spark mse_video_player.js

# Troubleshooting:  
  
- run with envvar GST_DEBUG="*:2" to troubleshoot gstreamer  
- ensure gstreamer correctly locates plugins (gst-play-1.0 <some-video-file>)  
- in macos if crash happens by assert !m_glDisplay - install xorg-server
- in macos 2 libglib can be linked twice (if you see in spark log "g_object_get_is_valid_property: object class '(null)' has no property named 'audio-sink'") - (check this by "otool -L Spark" it should output one line) - if not - remove /usr/local/opt
- black screen no video playing - in dash.js log if "codec is not installed" line found - install gstreamer1-gst-libav and gstreamer1-gst-plugins-* installed
- black screen no video playing - try to run with LIBGL_ALWAYS_SOFTWARE=1 (actually try to run LIBGL_ALWAYS_SOFTWARE glxinfo and LIBGL_ALWAYS_SOFTWARE=1 glxgears - they must work)
- black screen no video playing - try to run with LIBGL_ALWAYS_SOFTWARE=1 (actually try to run LIBGL_ALWAYS_SOFTWARE glxinfo and LIBGL_ALWAYS_SOFTWARE=1 glxgears - they must work)
