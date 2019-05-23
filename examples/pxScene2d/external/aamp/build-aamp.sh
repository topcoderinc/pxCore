
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/openssl/lib/pkgconfig

## libdash
rm -rf libdash
git clone git://github.com/bitmovin/libdash.git
cd libdash/libdash
mkdir build
cd build
cmake .. 
make -j4
cp bin/libdash.dylib /usr/local/lib/
mkdir -p /usr/local/include
mkdir -p /usr/local/lib/pkgconfig/
rm -rf /usr/local/include/libdash
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
cd ../../..
cp libdash-mac.pc /usr/local/lib/pkgconfig/libdash.pc
echo "libdash make and install successful!"


## aampabr
rm -rf aampabr
git clone https://code.rdkcentral.com/r/rdk/components/generic/aampabr aampabr
cd aampabr
mkdir build
cd build
cmake ..
make
make install

cd ../..
rm -rf libdash
rm -rf aampabr

export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/lib/:/usr/local/opt/openssl/lib:/usr/local/opt/gstreamer/lib:/usr/local/opt/glib/lib:/usr/local/opt/gettext/lib:/usr/local/opt/gst-plugins-base/lib:/usr/local/opt/ossp-uuid/lib
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/usr/local/include:/usr/local/opt/libxml2/include/libxml2:/usr/local/include/libdash
echo $PWD
## aamp
cd aamp
rm -rf build
mkdir build
cd build
cmake ..
make -j2
cd ../..
echo "aamp build success"
