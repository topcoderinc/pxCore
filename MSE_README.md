# Spark - HTML5 Video

1. download xcode 9.4.1 and 10.2.1 from https://developer.apple.com/download/more/, and decompress this two xip files, **spark only support 9.4.1, and brew only support new xcode version, so here must be need two version xcode until client update whole spark project**
2. make sure`git`, `cmake`,`cat`,`patch`,`python`(2.7.x) all are worked.
3. install brew https://brew.sh/

## Install libs

Xcode already include command tool, you may need remove it if you install command tools independent (https://macpaw.com/how-to/uninstall-xcode-on-macos). 

- First, we use xcode 10.2.1 install third libs, because some of didn't support 9.4.1

- ```
  sudo xcode-select -switch <your xcode 10.2.1 folder>/Xcode.app  # switch xcode version
  ```

  and then install third libs
  
  ```
  brew install gstreamer gst-validate gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly openssl libxml2 ossp-uuid gst-libav
  ```
  
  note: `gst-libav` is needed for avi format

### Build

spark only can build under xcode 9.4.1, so we need switch xcode version first.

```
sudo xcode-select -switch <your xcode 9.4.1 folder>/Xcode.app
```

below steps from here https://github.com/pxscene/pxCore/blob/master/examples/pxScene2d/README.md

**Note: you should clone new code from repo**

- `git clone https://github.com/topcoderinc/pxCore.git` (_tc_video branch)

- `cd pxCore`

- `cd examples/pxScene2d/external` run `./build.sh`

- then run below commands 

  ```
  cd pxCore
  mkdir temp 
  cd temp
  cmake ..
  make -j16
  ```

  after build, Spark.app will generated at *pxCore/examples/pxScene2d/src*

double check Spark.app , to run Spark.app

if you face nanosvg build error, you can

```
cd pxCore/examples/pxScene2d/external/nanosvg
./build.sh --force-clean
./build.sh
```
