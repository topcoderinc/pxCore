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
#include <iostream>
#include <string>
#include "pxHtmlVideo.h"

using namespace std::placeholders;

pxHtmlVideo *pxHtmlVideo::pxHtmlVideoObj = NULL;
GMainLoop *pxHtmlVideo::GstPlayerMainLoop;

extern pxContext context;


const std::vector<std::string> supportTypes{"m4v", "mp4", "avi"};

/**
 * @brief Thread to run mainloop (for standalone mode)
 * @param[in] arg user_data
 * @retval void pointer
 */
void *pxHtmlVideo::GstPlayer_StreamThread(void */*arg */)
{
  if (GstPlayerMainLoop) {
    g_main_loop_run(GstPlayerMainLoop); // blocks
    printf("GstPlayer_StreamThread: exited main event loop\n");
  }
  g_main_loop_unref(GstPlayerMainLoop);
  GstPlayerMainLoop = nullptr;
  return nullptr;
}

/**
 * @brief To initialize Gstreamer and start mainloop (for standalone mode)
 * @param[in] argc number of arguments
 * @param[in] argv array of arguments
 */
void pxHtmlVideo::InitPlayerLoop()
{
  if (!initialized) {
    initialized = true;
    gst_init(nullptr, nullptr);
    GstPlayerMainLoop = g_main_loop_new(nullptr, FALSE);
    mMainLoopThread = g_thread_new("GstPlayerLoop", &pxHtmlVideo::GstPlayer_StreamThread, nullptr);
  }
}

void pxHtmlVideo::TermPlayerLoop()
{
  if (GstPlayerMainLoop) {
    g_main_loop_quit(GstPlayerMainLoop);
    g_thread_join(mMainLoopThread);
    delete gPlayer;
    gst_deinit();
    printf("%s(): Exited GStreamer MainLoop.\n", __FUNCTION__);
  }
}

void pxHtmlVideo::newFrame(void *context, void */*data*/)
{
  auto *videoObj = reinterpret_cast<pxHtmlVideo *>(context);
  if (videoObj) {
    videoObj->onTextureReady();
  }
}

void pxHtmlVideo::newEvent(void *context, void *data)
{
  auto *videoObj = reinterpret_cast<pxHtmlVideo *>(context);
  const char *name = reinterpret_cast<const char *>(data);
  videoObj->mEmit.send(name, rtValue(name));
//  videoObj->mMediaSource->onEvent(name);
}


pxHtmlVideo::pxHtmlVideo(pxScene2d *scene) :
    pxObject(scene),
    mVideoTexture(),
    mAutoPlay(false),
    mUrl(""),
    gPlayer(nullptr),
    mTimeTik(0),
    mPreviousFrameTime(-1.0),
    previousFrame(nullptr),
    mMediaSource(new MediaSource)
{
  mMainLoopThread = nullptr;
  GstPlayerMainLoop = nullptr;
  InitPlayerLoop();

  // clean
  while (!frames.empty()) {
    frames.pop();
  }

  gPlayer = new GStreamPlayer(nullptr, [&](guint8 *buffer, int w, int h, guint /*stride*/, guint /*pixel_stride*/) {
    mFrameMutex.lock();
    guint8 *bits = buffer;
    auto *offscreen = new pxOffscreen;
    offscreen->init(w, h);  // mse frame size may changed when network changed, so need reset size

    for (int y = 0; y < h; y++) {
      pxPixel *d = offscreen->scanline(y);
      for (int x = 0; x < w; x++) {
        int index = y * w + x;
        *d = pxPixel(bits[index * 4], bits[index * 4 + 1], bits[index * 4 + 2]);
        d++;
      }
    }
    frames.push(offscreen);
    mFrameMutex.unlock();
    gUIThreadQueue->addTask(newFrame, pxHtmlVideo::pxHtmlVideoObj, nullptr);
  });
  gPlayer->setEventCallback([&](const char *event, void */*data*/) {
    gUIThreadQueue->addTask(newEvent, pxHtmlVideo::pxHtmlVideoObj, (void *) event);
  });
//  mMediaSource->setGstPlayer(gPlayer);
  rtLogError("GStreamer player created!");
  pxHtmlVideo::pxHtmlVideoObj = this;
}

pxHtmlVideo::~pxHtmlVideo()
{
  TermPlayerLoop();
}

void pxHtmlVideo::onInit()
{
  rtLogError("%s:%d.", __FUNCTION__, __LINE__);
  if (mAutoPlay) {
    play();
  }
  mReady.send("resolve", this);
  pxObject::onInit();
}

void pxHtmlVideo::dispose(bool /*pumpJavascript*/)
{
  delete gPlayer; // delete player, but don't stop g loop
  mFrameMutex.lock(); // clear all frames
  while (!frames.empty()) {
    pxOffscreen *frame = frames.front();
    delete frame;
    frames.pop();
  }
  mFrameMutex.unlock();
}

void pxHtmlVideo::draw()
{
  static pxTextureRef nullMaskRef;
  mFrameMutex.lock();

  if (!frames.empty()) {
    delete previousFrame;
    pxOffscreen *offscreen = frames.back(); // only fetch the last frame
    while (!frames.empty()) {
      pxOffscreen *t = frames.front();
      if (t != offscreen) {
        delete t;
      }
      frames.pop();
    }
    previousFrame = offscreen;
  }
  mFrameMutex.unlock();
  if (previousFrame) {
    context.drawImage(0, 0, mw, mh, context.createTexture(*previousFrame), nullMaskRef, false);
  }
}


void pxHtmlVideo::update(double t, bool updateChildren)
{
  if (mPreviousFrameTime <= 0) {
    mPreviousFrameTime = t;
  }


  mTimeTik += (t - mPreviousFrameTime);

  mEmit.send("timeupdate", rtValue("timeupdate"));
  if (mTimeTik > 0.2 && gPlayer->isLoaded() && gPlayer->isPlaying()) {
    mEmit.send("progress", rtValue("progress"));
    mTimeTik = 0;
  }

  mPreviousFrameTime = t;
  pxObject::update(t, updateChildren);
}

//properties
rtError pxHtmlVideo::duration(float &v) const
{
  v = gPlayer->getDuration();
  return RT_OK;
}


rtError pxHtmlVideo::position(float &v) const
{
  v = gPlayer->getPosition();
  return RT_OK;
}

rtError pxHtmlVideo::currentTime(float &v) const
{
  return this->position(v);
}

rtError pxHtmlVideo::setPosition(float v)
{
  gPlayer->setPosition(v);
  return RT_OK;
}

rtError pxHtmlVideo::url(rtString &url) const
{
  url = mUrl;
  return RT_OK;
}

rtError pxHtmlVideo::currentSrc(rtString &url) const
{
  return this->url(url);
}

rtError pxHtmlVideo::setUrl(const char *url)
{
  mUrl = rtString(url);
  rtLogError("%s:%d: URL[%s].", __FUNCTION__, __LINE__, url);

  if (!mUrl.isEmpty()) {
//    mMediaSource->addSourceBuffer(new SourceBuffer);
  }
  return RT_OK;
}

rtError pxHtmlVideo::autoPlay(bool &autoPlay) const
{
  autoPlay = mAutoPlay;
  return RT_OK;
}

rtError pxHtmlVideo::setAutoPlay(bool value)
{
  mAutoPlay = value;
  rtLogError("%s:%d: autoPlay[%s].", __FUNCTION__, __LINE__, value ? "TRUE" : "FALSE");
  gPlayer->setAutoPlay(mAutoPlay);
  return RT_OK;
}

rtError pxHtmlVideo::play()
{
  rtLogError("%s:%d.", __FUNCTION__, __LINE__);
  gPlayer->play();
  return RT_OK;
}

rtError pxHtmlVideo::pause()
{
  gPlayer->pause();
  return RT_OK;
}

rtError pxHtmlVideo::stop()
{
  gPlayer->release();
  return RT_OK;
}

rtError pxHtmlVideo::load()
{
  return RT_OK;
}

rtError pxHtmlVideo::canPlayType(rtString type, rtString &ret)
{
  std::string t = std::string(type.cString());
  for (const auto &supportType : supportTypes) {
    if (t.find(supportType) != std::string::npos) {
      ret = rtString("probably");
      return RT_OK;
    }
  }
  ret = rtString("");
  return RT_OK;
}

rtError pxHtmlVideo::videoTracks(rtObjectRef &v) const
{
  return RT_FAIL;
}

rtError pxHtmlVideo::audioTracks(rtObjectRef &v) const
{
  return RT_FAIL;
}

rtError pxHtmlVideo::ended(bool &ended) const
{
  return RT_OK;
}

rtError pxHtmlVideo::error(rtString &e) const
{
  return RT_OK;
}

rtError pxHtmlVideo::loop(bool &loop) const
{
  return RT_OK;
}

rtError pxHtmlVideo::setLoop(bool v)
{
  return RT_OK;
}

rtError pxHtmlVideo::paused(bool &p) const
{
  return RT_OK;
}

rtError pxHtmlVideo::readyState(int &v) const
{
  return mMediaSource->getReadyState(v);
}

rtError pxHtmlVideo::seekable(bool &v) const
{
  v = mMediaSource->seekable();
  return RT_OK;
}

rtError pxHtmlVideo::seeking(bool &v) const
{
  v = mMediaSource->isSeeking();
  return RT_OK;
}

rtError pxHtmlVideo::startDate(int64_t &v) const
{
  return RT_OK;
}


rtError pxHtmlVideo::getMediaSource(rtObjectRef &v) const
{
  v = mMediaSource;
  return RT_OK;
}

rtDefineObject(pxHtmlVideo, pxObject);
rtDefineProperty(pxHtmlVideo, audioTracks);
rtDefineProperty(pxHtmlVideo, autoPlay);
rtDefineProperty(pxHtmlVideo, currentSrc);
rtDefineProperty(pxHtmlVideo, currentTime);
rtDefineProperty(pxHtmlVideo, duration);
rtDefineProperty(pxHtmlVideo, ended);
rtDefineProperty(pxHtmlVideo, error);
rtDefineProperty(pxHtmlVideo, loop);
rtDefineProperty(pxHtmlVideo, paused);
rtDefineProperty(pxHtmlVideo, readyState);
rtDefineProperty(pxHtmlVideo, seekable);
rtDefineProperty(pxHtmlVideo, seeking);
rtDefineProperty(pxHtmlVideo, src);
rtDefineProperty(pxHtmlVideo, startDate);
rtDefineProperty(pxHtmlVideo, videoTracks);
rtDefineProperty(pxHtmlVideo, position);
rtDefineProperty(pxHtmlVideo, mediaSource);

rtDefineMethod(pxHtmlVideo, play);
rtDefineMethod(pxHtmlVideo, pause);
rtDefineMethod(pxHtmlVideo, stop);
rtDefineMethod(pxHtmlVideo, load);
rtDefineMethod(pxHtmlVideo, canPlayType);

