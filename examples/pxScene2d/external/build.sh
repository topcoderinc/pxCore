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

#--------- CURL

make_parallel=3

if [ "$(uname)" = "Darwin" ]; then
    make_parallel="$(sysctl -n hw.ncpu)"
elif [ "$(uname)" = "Linux" ]; then
    make_parallel="$(cat /proc/cpuinfo | grep '^processor' | wc --lines)"
fi

if [ ! -e ./curl/lib/.libs/libcurl.4.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "CURL"

  cd curl

  if [ "$(uname)" = "Darwin" ]; then
    ./configure --with-darwinssl
    #Removing api definition for Yosemite compatibility.
    sed -i '' '/#define HAVE_CLOCK_GETTIME_MONOTONIC 1/d' lib/curl_config.h
  else
      if [ $(echo "$(openssl version | cut -d' ' -f 2 | cut -d. -f1-2)>1.0" | bc) ]; then
          echo "Openssl is too new for this version of libcurl.  Opting for gnutls instead..."
          ./configure --with-gnutls
      else
          echo "Using openssl < 1.1.*"
          ./configure --with-ssl
      fi

      if [ "$(cat config.log | grep '^SSL_ENABLED' | cut -d= -f 2)" != "'1'" ]; then
          echo "Failed to configure libcurl with SSL support" && exit 1
      fi
  fi

  
  make all "-j${make_parallel}"
  cd ..

fi

#--------- PNG

if [ ! -e ./libpng-1.6.28/.libs/libpng16.16.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "PNG"

  cd png
  ./configure
  make all "-j${make_parallel}"
  cd ..

fi

#--------- GIF

#if [ ! -e ./gif/.libs/libgif.dylib ] &&
#[ "$(uname)" != "Darwin" ]
#then

#banner "GIF"
#cd gif
#sudo make install
#[ -d .libs ] || mkdir -p .libs
#if [ -e libgif.dylib ]
#then
#cp libgif.dylib .libs/libgif.dylib
#cp libutil.dylib .libs/libgifutil.dylib

#elif [ -e libgif.so ]
#then
#cp libgif.so .libs/libgif.dylib
#cp libutil.so .libs/libgifutil.dylib
#fi

#cd ..
#fi

#--------- FT

#if [ ! -e ./ft/objs/.libs/libfreetype.6.dylib ] ||
#   [ "$(uname)" != "Darwin" ]
#then
#
#  banner "FT"
#
#  cd ft
#  export LIBPNG_LIBS="-L../png/.libs -lpng16"
#  ./configure --with-png=no
#  make all "-j${make_parallel}"
#  cd ..
#
#fi

#--------- JPG

if [ ! -e ./jpg/.libs/libjpeg.9.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "JPG"

  cd jpg
  ./configure
  make all "-j${make_parallel}"
  cd ..

fi

#--------- ZLIB

if [ ! -e ./zlib/libz.1.2.11.dylib ] ||
   [ "$(uname)" != "Darwin" ]
then

  banner "ZLIB"

  cd zlib
  ./configure
  make all "-j${make_parallel}"
  cd ..

fi

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
  ./configure
  make "-j${make_parallel}"
  cd ..

fi

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

#-------- NANOSVG

  banner "NANOSVG"

./nanosvg/build.sh

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
   ./Tools/gtk/install-dependencies
   ./Tools/Scripts/build-webkit --gtk --debug --media-source --video --video-track --system-malloc --cmakeargs ' -DUSE_WPE_RENDERER=OFF -DUSE_WOFF2=OFF -DUSE_OPENJPEG=OFF -DENABLE_INTROSPECTION=OFF -DENABLE_BUBBLEWRAP_SANDBOX=OFF'  --no-web-crypto --no-web-rtc --no-media-stream  --no-spellcheck --web-audio
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
