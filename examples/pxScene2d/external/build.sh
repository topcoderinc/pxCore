#!/bin/bash
set -e
# Any subsequent(*) commands which fail will cause the shell script to exit immediately

banner() {
  msg="# $* #"
  edge=$(echo "$msg" | sed 's/./#/g')
  echo " "
  echo "$edge"
  echo "$msg"
  echo "$edge"
  echo " "
}

EXT_INSTALL_PATH=$PWD/extlibs
mkdir -p $EXT_INSTALL_PATH

make_parallel=3

if [ "$(uname)" = "Darwin" ]; then
    make_parallel="$(sysctl -n hw.ncpu)"
    LIBEXTN=dylib
elif [ "$(uname)" = "Linux" ]; then
    make_parallel="$(cat /proc/cpuinfo | grep '^processor' | wc --lines)"
    LIBEXTN=so
fi

#--------- PNG

if [ ! -e ./libpng-1.6.28/.libs/libpng16.16.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "PNG"

  cd png
  ./configure --prefix=$EXT_INSTALL_PATH
  make all "-j${make_parallel}"
  make install
  cd ..

fi
#---------

#--------- JPG

if [ ! -e ./jpg/.libs/libjpeg.9.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "JPG"

  cd jpg
  ./configure --prefix=$EXT_INSTALL_PATH
  make all "-j${make_parallel}"
  make install
  cd ..

fi
#---------

if [[ $# -eq 1 ]] && [[ $1 == "SPARK_ENABLE_VIDEO" ]]; then
#--------graphite2

if [ ! -e $EXT_INSTALL_PATH/lib/libgraphite2.la ]
then
  banner "graphite2"

  ./graphite2/build.sh
fi

#--------

#-------- pcre

if [ ! -e $EXT_INSTALL_PATH/lib/libpcre.la ]
then
  banner "pcre"

  ./pcre/build.sh
fi

#--------

#--------icu

if [ ! -e $EXT_INSTALL_PATH/lib/libicudata.$LIBEXTN ]
then
  banner "icu"

  ./icu/build.sh
fi

#--------

#-------- libffi

if [ ! -e $EXT_INSTALL_PATH/lib/libffi.la ]
then
  banner "libffi"

  ./libffi/build.sh
fi

#--------

#--------gettext

if [ ! -e $EXT_INSTALL_PATH/lib/libintl.la ]
then
  banner "gettext"

  ./gettext/build.sh
fi

#--------

#--------glib

if [ ! -e $EXT_INSTALL_PATH/lib/libglib-2.0.la ]
then
  banner "glib"

  ./glib/build.sh
fi

#--------
fi

#--------- FT

if [ ! -e ./ft/objs/.libs/libfreetype.6.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "FT"

  cd ft
  LIBPNG_LIBS="-L../png/.libs -lpng16" PKG_CONFIG_PATH=$EXT_INSTALL_PATH/lib/pkgconfig:$PKG_CONFIG_PATH ./configure --with-png=no --prefix=$EXT_INSTALL_PATH
  make all "-j${make_parallel}"
  make install
  cd ..

fi
#---------

if [[ $# -eq 1 ]] && [[ $1 == "SPARK_ENABLE_VIDEO" ]]; then
#--------Fontconfig

if [ ! -e $EXT_INSTALL_PATH/lib/libfontconfig.$LIBEXTN ]
then
  banner "Fontconfig"

  ./fontconfig/build.sh
fi

#--------

#--------harfbuzz

if [ ! -e $EXT_INSTALL_PATH/lib/libharfbuzz.la ]
then
  banner "harfbuzz"

  ./harfbuzz/build.sh
fi
#---------
fi

#-------- openssl

if [ ! -e $EXT_INSTALL_PATH/lib/libcrypto.$LIBEXTN ]
then
  banner "openssl"

  ./openssl/build.sh
fi

#--------

#--------- CURL

if [ ! -e $EXT_INSTALL_PATH/lib/libcurl.la ]; then

  banner "CURL"

  cd curl

  PKG_CONFIG_PATH=$EXT_INSTALL_PATH/lib/pkgconfig:$PKG_CONFIG_PATH ./configure --with-ssl --prefix=$EXT_INSTALL_PATH

  if [ "$(uname)" = "Darwin" ]; then
    #Removing api definition for Yosemite compatibility.
    sed -i '' '/#define HAVE_CLOCK_GETTIME_MONOTONIC 1/d' lib/curl_config.h
  fi

  
  make all "-j${make_parallel}"
  make install
  cd ..

fi
#---------

#--------- ZLIB

if [ ! -e ./zlib/libz.1.2.11.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "ZLIB"

  cd zlib
  ./configure --prefix=$EXT_INSTALL_PATH
  make all "-j${make_parallel}"
  make install
  cd ..

fi
#---------

#--------- LIBJPEG TURBO (Non -macOS)

if [ "$(uname)" != "Darwin" ]
then

  banner "TURBO"

  cd libjpeg-turbo
  git update-index --assume-unchanged Makefile.in
  git update-index --assume-unchanged aclocal.m4
  git update-index --assume-unchanged ar-lib
  git update-index --assume-unchanged compile
  git update-index --assume-unchanged config.guess
  git update-index --assume-unchanged config.h.in
  git update-index --assume-unchanged config.sub
  git update-index --assume-unchanged configure
  git update-index --assume-unchanged depcomp
  git update-index --assume-unchanged install-sh
  git update-index --assume-unchanged java/Makefile.in
  git update-index --assume-unchanged ltmain.sh
  git update-index --assume-unchanged md5/Makefile.in
  git update-index --assume-unchanged missing
  git update-index --assume-unchanged simd/Makefile.in

  autoreconf -f -i
  ./configure --prefix=$EXT_INSTALL_PATH
  make "-j${make_parallel}"
  make install
  cd ..

fi
#---------

#--------- LIBNODE

if [ ! -e node/libnode.dylib ] ; then

  banner "NODE"

  cd node
  ./configure --shared
  make "-j${make_parallel}"
  ln -sf out/Release/obj.target/libnode.so.48 libnode.so.48
  ln -sf libnode.so.48 libnode.so
  ln -sf out/Release/libnode.48.dylib libnode.48.dylib
  ln -sf libnode.48.dylib libnode.dylib
  cd ..
fi
#---------

#--------- uWebSockets
if [ ! -e ./uWebSockets/libuWS.dylib ]
then

  banner "uWEBSOCKETS"

  cd uWebSockets

  if [ -e Makefile.build ]
  then
    mv Makefile.build Makefile
  fi

  make
  cd ..
fi
#--------

# v8
# todo - uncomment - for now build v8 with buildV8.sh directly
#bash buildV8.sh

#-------- BREAKPAD (Non -macOS)

if [ "$(uname)" != "Darwin" ]; then
  ./breakpad/build.sh
fi
#---------

#-------- NANOSVG

  banner "NANOSVG"

./nanosvg/build.sh
#---------

#-------- DUKTAPE

if [ ! -e dukluv/build/libduktape.a ]
then
  banner "DUCKTAPE"

  ./dukluv/build.sh
fi

#--------
# webkit
if [ "$(uname)" == "Linux" ]
then
   git clone https://github.com/WebKit/webkit.git
   cd webkit
   git checkout ad02ed4fc62ca969b96ea99fdedef3a9153914d4
   git apply < ../0001-webkit_mse_fix.patch
   git apply < ../0001-webkit-pause-support.patch
   git apply < ../0001-webkit-build-libs-as-shared.patch
   git apply < ../0001-fixes-to-use-gst-pipeline-from-pxcore.patch
   ./Tools/gtk/install-dependencies
   ./Tools/Scripts/build-webkit --gtk --media-source --video --video-track --system-malloc --cmakeargs ' -DUSE_WPE_RENDERER=OFF -DUSE_WOFF2=OFF -DUSE_OPENJPEG=OFF -DENABLE_INTROSPECTION=OFF -DENABLE_BUBBLEWRAP_SANDBOX=OFF -DCMAKE_CXX_FLAGS="-fdata-sections -ffunction-sections -Os" ' --no-accelerated-2d-canvas --no-accelerated-overflow-scrolling --no-apple-pay --no-apple-pay-session-v3 --no-apple-pay-session-v4 --no-application-manifest --no-async-scrolling --no-attachment-element --no-avf-captions --no-bubblewrap-sandbox --no-cloop --no-cache-partitioning --no-channel-messaging --no-content-extensions --no-content-filtering --no-context-menus --no-css3-text --no-css-box-decoration-break --no-css-compositing --no-css-conic-gradients --no-css-device-adaptation --no-css-image-orientation --no-css-image-resolution --no-css-painting-api --no-css-scroll-snap --no-css-selectors-level4 --no-css-typed-om --no-cursor-visibility --no-custom-scheme-handler --no-dark-mode-css --no-datacue-value --no-datalist-element --no-data-interaction --no-device-orientation --no-dfg-jit --no-download-attribute --no-drag-support --no-encrypted-media --no-fast-jit-permissions --no-fetch-api --no-filters-level-2 --no-ftl-jit --no-ftpdir --no-fullscreen-api --no-gamepad --no-geolocation --no-indexed-database --no-indexed-database-in-workers --no-input-type-color --no-input-type-date --no-input-type-datetimelocal --no-input-type-datetime --no-input-type-month --no-input-type-time --no-input-type-week --no-inspector-alternate-dispatchers --no-intl --no-ios-gesture-events --no-ios-touch-events --no-jit --no-layout-formatting-context --no-legacy-css-vendor-prefixes --no-legacy-custom-protocol-manager --no-legacy-encrypted-media --no-letterpress --no-mac-gesture-events --no-mathml --no-media-capture --no-media-session --no-media-statistics --no-media-stream --no-memory-sampler --no-meter-element --no-mhtml --no-mouse-cursor-scale --no-netscape-plugin-api --no-notifications --no-orientation-events --no-payment-request --no-pdfkit-plugin --no-pointer-lock --no-remote-inspector --no-resolution-media-query --no-resource-load-statistics --no-resource-usage --no-rubber-banding --no-sampling-profiler --no-service-controls --no-service-worker --no-speech-synthesis --no-spellcheck --no-svg-fonts --no-telephone-number-detection --no-text-autosizing --no-touch-events --no-touch-slider --no-userselect-all --no-user-message-handlers --no-variation-fonts --no-video-presentation-mode --no-video-uses-element-fullscreen --no-quota --no-webassembly --no-webassembly-streaming-api --no-webdriver --no-webgpu --no-web-authn --no-web-crypto --no-web-rtc --no-wireless-playback-target --no-xslt
   cd ..
elif [ "$(uname)" == "Darwin" ]; then
   git clone https://github.com/WebKit/webkit.git
   cd webkit
   git checkout ad02ed4fc62ca969b96ea99fdedef3a9153914d4
   git apply < ../0001-osx-webkit-fixes.patch
   sudo port install webkit-gtk
   sudo port install 'gstreamer1-*'
   ./Tools/Scripts/build-webkit --gtk --debug --media-source --video --video-track --cmakeargs ' -DUSE_WPE_RENDERER=OFF -DUSE_WOFF2=OFF -DUSE_OPENJPEG=OFF -DENABLE_INTROSPECTION=OFF -DENABLE_BUBBLEWRAP_SANDBOX=OFF -DCMAKE_EXE_LINKER_FLAGS="-L /opt/local/lib" -DCMAKE_MODULE_LINKER_FLAGS="-L /opt/local/lib" -DCMAKE_SHARED_LINKER_FLAGS="-L /opt/local/lib"  '  --no-web-crypto --no-web-rtc --no-media-stream  --no-spellcheck --web-audio
   cd ..
fi

# dash
if [ ! -e dash.js-3.0.0/dist/dash.all.debug.pxcore.js ]
then
    bash buildDashJS.sh
fi

# shaka
if [ ! -e shaka-player/dist/shaka-player.compiled.debug.js ]
then
    bash buildShakaJS.sh
fi

# libdash
if [ ! -e extlibs/lib/libdash.so ]
then
    bash ./libdash/libdash/build.sh
fi

# aampabr
if [ ! -e extlibs/lib/libabr.so ]
then
    bash ./aampabr/build.sh
fi

# gst-pipeline
if [ ! -e gst-pipeline/build/libgstpipeline.a ]
then
    bash ./gst-pipeline/build.sh
fi

# aamp
if [ ! -e extlibs/lib/libaamp.so ]
then
    bash ./aamp/build.sh
fi

if [[ $# -eq 1 ]] && [[ $1 == "SPARK_ENABLE_VIDEO" ]]; then
#-------- cJSON

if [ ! -e $EXT_INSTALL_PATH/lib/libcjson.$LIBEXTN ]
then
  banner "cJSON"

  ./cJSON/build.sh
fi

#--------

#--------orc

if [ ! -e $EXT_INSTALL_PATH/lib/liborc-0.4.la ]
then
  banner "orc"

  ./orc/build.sh
fi

#--------


#--------ossp-uuid

if [ ! -e $EXT_INSTALL_PATH/lib/libuuid.la ]
then
  banner "ossp-uuid"

  ./ossp-uuid/build.sh
fi

#--------

#--------libxml2

if [ ! -e $EXT_INSTALL_PATH/lib/libxml2.la ]
then
  banner "libxml2"

  ./libxml2/build.sh
fi

#--------

#-------- libdash

if [ ! -e $EXT_INSTALL_PATH/lib/libdash.$LIBEXTN ]
then
  banner "libdash"

  ./libdash/libdash/build.sh
fi

#--------

#-------- xz-5.2.2

if [ ! -e $EXT_INSTALL_PATH/lib/liblzma.la ]
then
  banner "xz"

  ./xz/build.sh
fi

#--------

#-------- gstreamer-1.16

if [ ! -e $EXT_INSTALL_PATH/lib/libgstreamer-1.0.la ]
then
  banner "gstreamer-1.16"

  ./gstreamer/build.sh
fi

#--------

#-------- gst-plugin-base

if [ ! -e $EXT_INSTALL_PATH/lib/libgstapp-1.0.la ]
then
  banner "gst-plugins-base"

  ./gst-plugins-base/build.sh
fi

#--------
#-------- gst-plugin-bad

if [ ! -e $EXT_INSTALL_PATH/lib/libgstbadaudio-1.0.la ]
then
  banner "gst-plugins-bad"

  ./gst-plugins-bad/build.sh
fi

#--------
#-------- gst-plugin-ugly

if [ ! -e $EXT_INSTALL_PATH/lib/gstreamer-1.0/libgstx264.la ]
then
  banner "gst-plugins-ugly"

  ./gst-plugins-ugly/build.sh
fi

#--------
#-------- gst-plugin-good

if [ ! -e $EXT_INSTALL_PATH/lib/gstreamer-1.0/libgstavi.la ]
then
  banner "gst-plugins-good"

  ./gst-plugins-good/build.sh
fi

#--------
#-------- gst-libav

if [ ! -e $EXT_INSTALL_PATH/lib/gstreamer-1.0/libgstlibav.la ]
then
  banner "gst-libav"

  ./gst-libav/build.sh
fi

#--------

#-------- aampabr

if [ ! -e $EXT_INSTALL_PATH/lib/libabr.$LIBEXTN ]
then
  banner "aampabr"

  ./aampabr/build.sh
fi

#--------

#-------- aamp

if [ ! -e $EXT_INSTALL_PATH/lib/libaamp.$LIBEXTN ]
then
  banner "aamp"

  ./aamp/build.sh
fi

#--------
fi
