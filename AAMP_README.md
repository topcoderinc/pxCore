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

- `git clone https://github.com/topcoderinc/pxCore.git`

- `cd pxCore`

- `git checkout 7e7b39b06de7b0b5f92f0c2e550f802d46a2071b`

- `git apply aamp.patch`

- `cd examples/pxScene2d/external` run `./build.sh`

- `cd aamp` and run `./build-aamp.sh`

- then run below commands (dukluv build failed in this repo, we need use `-DSUPPORT_DUKTAPE=OFF` skip it )

  ```
  cd pxCore
  mkdir temp 
  cd temp
  cmake .. -DSUPPORT_DUKTAPE=OFF 
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

- we need same video frame data into rbg array, then pass rgb array to spark opengl, this in worked perfect on 1920*1080 video or belows, but for 2K/4K video, it has a bit lag.

## AAMP issues

1. seek problem, it worked on some mdp source but very slow, and not worked on some mdp, like this [http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd](http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd)

2. get current time from aamp-cli is not worked, in priv_aamp.h line 1911 method "GetPositionMs", it returned wrong play position in all dash url (few seconds before real play time and total wrong if seek), i notice aamp used below codes (iteration time diff * speed)

main_aamp.cpp line 361

`long long elapsedTime = aamp_GetCurrentTimeMS() - trickStartUTCMS; # iteration time diff`
`eventData.data.progress.positionMiliseconds += elapsedTime * rate; # time diff * speed`

i think the wrong reason maybe from here, not sure how to get the real play time.

3. get total time of video is not worked on some mdp, in priv_aamp.h line 1904 method "GetDurationMs"
   for my test, these two can get right "Duration"
   [http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd](http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd)
   [http://dash.edgesuite.net/akamai/bbb_30fps/bbb_30fps.mpd](http://dash.edgesuite.net/akamai/bbb_30fps/bbb_30fps.mpd)
   but for this [http://bitmovin-a.akamaihd.net/content/playhouse-vr/mpds/105560.mpd,](http://bitmovin-a.akamaihd.net/content/playhouse-vr/mpds/105560.mpd,) it cannot get right "Duration".

## Validation

- https://github.com/topcoderinc/pxCore/issues/284

  open spark app, enter `file://<submission_folder>/testVideo.js` to see overlay and animation for video

- https://github.com/topcoderinc/pxCore/issues/285

  Enter `http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd` in url input box

  - seek may not worked as expected, see issue 1
  - Current time may not worked as expected (when pause or seek the current time will wrong)
  - after finished video play,   use command to see  `cat /var/tmp/Spark.log`, you will see log *[SPARK LOG] >>>>>> on sourceended*, this mean video finished play

  

  video https://youtu.be/calgh4S-x28

## Test

follow readme.md to build Spark.app



after build successful, there is an **aamp-cli** in pxCore/examples/pxScene2d/external/aamp/aamp/build

you can use `./aamp-cli` then copy video url to test, for now, aamp looks didn't support mp4 format



open **Spark.app**, use those url to play 

* http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd
* http://yt-dash-mse-test.commondatastorage.googleapis.com/media/oops-20120802-manifest.mpd
* http://bitmovin-a.akamaihd.net/content/playhouse-vr/mpds/105560.mpd (4K video)
* http://dash.edgesuite.net/akamai/bbb_30fps/bbb_30fps.mpd
* http://bitmovin-a.akamaihd.net/content/playhouse-vr/m3u8s/105560.m3u8 (4K video)



Note: not all of dash url are support, you need put it in aamp-cli first, if aamp-cli support, spark also should be support
