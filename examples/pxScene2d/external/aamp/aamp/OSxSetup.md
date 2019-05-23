---

# AAMP on Mac OS X

This document contains the instructions to setup and debug stand alone AAMP (aamp-cli) on Mac OS X.

## Install dependancies

**1. Install XCode from the Apple Store**

**2. Install XCode Command Line Tools**

```c
xcode-select --install
sudo installer -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_<version>.pkg -target /
```
**3. Install GStreamer packages**

Install Homebrew, if not available in your Mac:
```c
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"$
```

Install gst packages:
```c
brew install gstreamer gst-validate gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly
```

You can find help [here](https://wesleyli.co/2016/10/running-gstreamer-on-mac-os-x).
More details about packages available at [freedesktop.org](https://gstreamer.freedesktop.org/documentation/installing/on-mac-osx.html)

**4. Install [Cmake](https://cmake.org/download/)**

**5. Install OpenSsl**

```c
brew install openssl
sudo ln -s /usr/local/Cellar/openssl/<version> /usr/local/ssl
```
**6. Install libXML2**

```c
brew install libxml2
```
**7. Install libdash**

***Build***:

```c
git clone git://github.com/bitmovin/libdash.git
cd libdash/libdash
mkdir build
cd build
cmake ..
make
```
***Install***:

```c
cp bin/libdash.dylib /usr/local/lib/
mkdir /usr/local/include
mkdir /usr/local/include/libdash
mkdir /usr/local/include/libdash/xml
mkdir /usr/local/include/libdash/mpd
mkdir /usr/local/include/libdash/helpers
mkdir /usr/local/include/libdash/network
mkdir /usr/local/include/libdash/portable
mkdir /usr/local/include/libdash/metrics
cp -pr ../libdash/include/*.h /usr/local/include/libdash
cp -pr ../libdash/source/xml/*.h /usr/local/include/libdash/xml
cp -pr ../libdash/source/mpd/*.h /usr/local/include/libdash/mpd
cp -pr ../libdash/source/network/*.h /usr/local/include/libdash/network
cp -pr ../libdash/source/portable/*.h /usr/local/include/libdash/portable
cp -pr ../libdash/source/helpers/*.h /usr/local/include/libdash/helpers
cp -pr ../libdash/source/metrics/*.h /usr/local/include/libdash/metrics

echo -e 'prefix=/usr/local \nexec_prefix=${prefix} \nlibdir=${exec_prefix}/lib \nincludedir=${prefix}/include/libdash \n \nName: libdash \nDescription: ISO/IEC MPEG-DASH library \nVersion: 3.0 \nRequires: libxml-2.0 \nLibs: -L${libdir} -ldash \nLibs.private: -lxml2 \nCflags: -I${includedir}'  > /usr/local/lib/pkgconfig/libdash.pc
```
**8. Install libuuid**

```c
brew install ossp-uuid
```
**9. Install aampabr**

```c
git clone https://code.rdkcentral.com/r/rdk/components/generic/aampabr aampabr
cd aampabr
mkdir build
cd build
cmake ..
make
make install
```
##Build and execute aamp-cli
**1. Open aamp.xcodeproj in Xcode**

**2. Build the code**

```c
	Product -> Build
```
**3. Select target to execute**

```c
	Product -> Scheme -> Edit scheme
	Run page-> Info
	Select Executable -> Other and open the ‘aamp-cli’ image name from {AAMP_PATH}/build/aamp_cli
```
**4. Execute**

```c
Product -> Run
```
