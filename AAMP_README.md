# Hercules C++ Spark App - AAMP media player integration

1. download xcode 9.4.1 and 10.2.1 from https://developer.apple.com/download/more/, and decompress this two xip files
2. make sure`git`, `cmake`,`cat`,`patch`,`python`(2.7.x) all are worked.
3. install brew https://brew.sh/

## Install libs

- First, we use xcode 10.2.1 install third libs, because some of didn't support 9.4.1

- ```
  sudo xcode-select -switch <your xcode 10.2.1 folder>/Xcode.app  # switch xcode version
  ```

  and then install third libs
  ```
  brew install gstreamer gst-validate gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly openssl libxml2 ossp-uuid
  ```

### Build

spark only can build under xcode 9.4.1, so we need switch xcode version first.

```
sudo xcode-select -switch <your xcode 9.4.1 folder>/Xcode.app
```

below steps from here https://github.com/pxscene/pxCore/blob/master/examples/pxScene2d/README.md

**Note: you should clone new code from repo**

- `git clone https://github.com/pxscene/pxCore.git`

- `cd pxCore`

- `git checkout 444a5f0e5f3575089616be4c1016ad855f29fa01`

- `git apply aamp.patch`

- `cd pxCore/examples/pxScene2d/external` run `./build.sh`

- `cd aamp` and run `./build-aamp.sh`

- then 

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



## Notes

- aamp need update libpng, jpeg,and freetype, i updated this three libs in external
- aamp will red first when load


follow readme.md to build Spark.app



after build successful, there is an **aamp-cli** in pxCore/examples/pxScene2d/external/aamp/aamp/build

you can use `./aamp-cli` then copy video url to test, for now, aamp looks didn't support mp4 format



open **Spark.app**, use those url to play 

* http://bitmovin-a.akamaihd.net/content/playhouse-vr/mpds/105560.mpd
* http://dash.edgesuite.net/akamai/bbb_30fps/bbb_30fps.mpd
* http://bitmovin-a.akamaihd.net/content/playhouse-vr/m3u8s/105560.m3u8

click video to get focus, the press "Space" button in keyboard to pause or play video.

Note: not all of dash url are support, you need put it in aamp-cli first, if aamp-cli support, spark also should be support

## Video

https://youtu.be/bsyJkO-AoqQ
