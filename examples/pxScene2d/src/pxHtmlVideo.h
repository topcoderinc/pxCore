/*

 pxCore Copyright 2005-2018 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#ifndef PX_HTML_VIDEO_H
#define PX_HTML_VIDEO_H

#include <gst/gst.h>
#include "mse/GStreamPlayer.h"
#include "pxScene2d.h"
#include "pxObject.h"
#include "pxContext.h"
#include "mse/MediaSource.h"
#include <queue>

class pxHtmlVideo : public pxObject {
public:
  rtDeclareObject(pxHtmlVideo, pxObject);


  rtReadOnlyProperty(audioTracks, audioTracks, rtObjectRef);

  rtProperty(autoPlay, autoPlay, setAutoPlay, bool);

  rtReadOnlyProperty(currentSrc, currentSrc, rtString);

  rtReadOnlyProperty(currentTime, currentTime, float);

  rtReadOnlyProperty(duration, duration, float);

  rtReadOnlyProperty(ended, ended, bool);

  rtReadOnlyProperty(error, error, rtString);

  rtProperty(loop, loop, setLoop, bool);

  rtReadOnlyProperty(paused, paused, bool);

  rtReadOnlyProperty(readyState, readyState, int);

  rtReadOnlyProperty(seekable, seekable, bool);

  rtReadOnlyProperty(seeking, seeking, bool);

  rtProperty(src, url, setUrl, rtString);

  rtReadOnlyProperty(startDate, startDate, int64_t);

  rtReadOnlyProperty(videoTracks, videoTracks, rtObjectRef);


  rtProperty(position, position, setPosition, float);
  rtReadOnlyProperty(mediaSource, getMediaSource, rtObjectRef);
  // methods
  rtMethodNoArgAndNoReturn("play", play);

  rtMethod1ArgAndReturn("canPlayType", canPlayType, rtString, rtString);

  rtMethodNoArgAndNoReturn("pause", pause);

  rtMethodNoArgAndNoReturn("stop", stop);

  rtMethodNoArgAndNoReturn("load", load);


  pxHtmlVideo(pxScene2d *scene);

  virtual ~pxHtmlVideo();

  virtual void onInit();

  //properties
  rtError audioTracks(rtObjectRef &v) const;

  rtError videoTracks(rtObjectRef &v) const;

  virtual rtError duration(float &v) const;


  virtual rtError position(float &v) const;

  virtual rtError currentTime(float &v) const;

  virtual rtError setPosition(float v);


  virtual rtError url(rtString &url) const;

  virtual rtError currentSrc(rtString &url) const;

  virtual rtError error(rtString &url) const;

  virtual rtError setUrl(const char *s);

  virtual rtError startDate(int64_t &v) const;

  virtual rtError autoPlay(bool &autoPlay) const;

  virtual rtError setAutoPlay(bool v);

  virtual rtError ended(bool &ended) const;

  virtual rtError paused(bool &p) const;

  virtual rtError loop(bool &loop) const;

  virtual rtError setLoop(bool v);

  virtual rtError readyState(int &v) const;

  virtual rtError seekable(bool &v) const;

  virtual rtError seeking(bool &v) const;

  virtual rtError getMediaSource(rtObjectRef &v) const;
  //methods
  virtual rtError play();

  virtual rtError load();

  virtual rtError canPlayType(rtString type, rtString &ret);

  virtual rtError pause();

  virtual rtError stop();


  virtual void draw();

  void update(double t, bool updateChildren) override;

  /**
   * when pxObject dispose
   * @param pumpJavascript from js invoke ?
   */
  virtual void dispose(bool pumpJavascript);

  static void newFrame(void *context, void *data);

  static void newEvent(void *context, void *data);

private:


  void InitPlayerLoop();

  void TermPlayerLoop();

  static void *GstPlayer_StreamThread(void *arg);


private:

  static GMainLoop *GstPlayerMainLoop;

  pxTextureRef mVideoTexture;
  bool mAutoPlay;
  rtString mUrl;

  rtMutex mFrameMutex;
  bool initialized = false;
  GThread *mMainLoopThread;
  GStreamPlayer *gPlayer;
  double mTimeTik;
  double mPreviousFrameTime;
  std::queue<pxOffscreen *> frames;
  pxOffscreen *previousFrame;
  MediaSource *mMediaSource;


public:
  static pxHtmlVideo *pxHtmlVideoObj; //This object
};

#endif // PX_HTML_VIDEO_H
