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
#include "pxHtmlVideo2.h"

#ifdef ENABLE_SPARK_VIDEO
#include "WebCore/config.h"
#include "MediaSource.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"
#include "WebCore/dom/Event.h"
#include "WebCore/dom/EventNames.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#endif

#include "mse/MSEMediaSource.h"
#include "mse/MSEWebKitDocument.h"

extern pxContext context;

pxHtmlVideo2 *pxHtmlVideo2::pxHtmlVideoObj = NULL;

struct pxHtmlVideo2Impl {
  pxHtmlVideo2Impl(WebCore::HTMLVideoElement &videoElement)
    : mVideoElement(videoElement)
  {
  }

  Ref<WebCore::HTMLVideoElement> mVideoElement;
};

void pxHtmlVideo2::onRepaint(void *context, void */*data*/)
{
  auto *videoObj = reinterpret_cast<pxHtmlVideo2 *>(context);
  if (videoObj) {
    videoObj->onTextureReady();
  }
}

static void webkitOnFrameRendered(void *arg, void *sampleArg)
{
  pxHtmlVideo2 *htmlVideoObj = (pxHtmlVideo2*)arg;
  GstSample *sample = (GstSample *)sampleArg;
  GstVideoFrame v_frame;
  GstVideoInfo v_info;
  GstBuffer *buf = gst_sample_get_buffer(sample);
  GstCaps *caps = gst_sample_get_caps(sample);
  gst_video_info_from_caps(&v_info, caps);

  if (gst_video_frame_map(&v_frame, &v_info, buf, (GstMapFlags) (GST_MAP_READ))) {
    int height = GST_VIDEO_FRAME_HEIGHT(&v_frame);
    int width = GST_VIDEO_FRAME_WIDTH(&v_frame);

    guint8 *pixels = static_cast<guint8 *>GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
    guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&v_frame, 0);
    guint pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (&v_frame, 0);

    htmlVideoObj->onNewFrame(pixels, width, height, stride, pixel_stride);

    gst_video_frame_unmap(&v_frame);
  }
}

pxHtmlVideo2::pxHtmlVideo2(pxScene2d *scene) :
    pxObject(scene), previousFrame(nullptr),
    mNetworkState(0),
    mReadyState(0),
    mSeeking(0),
    mCurrentTime(0),
    mDuration(0),
    mPaused(false),
    mDefaultPlaybackRate(0),
    mPlaybackRate(0),
    mEnded(false),
    mAutoplay(false),
    mLoop(false),
    mControls(false),
    mVolume(false),
    mMuted(false),
    mDefaultMuted(false)
{
  Ref<WebCore::HTMLVideoElement> videoElement = WebCore::HTMLVideoElement::create(MSEWebkitDocument::get().getDocument());
  videoElement->setRenderFrameCallback(&webkitOnFrameRendered, (void*)this);

  for (auto s: {"canplay", "play", "waiting", "playing", "pause", "error", "seeking", "seeked", "timeupdate",
    "progress", "ratechange", "loadedmetadata", "stalled", "ended"})
  {
    addWebkitEventListener(&videoElement.get(), s);
  }

  mVideoImpl = new pxHtmlVideo2Impl(videoElement.get());

  pxHtmlVideo2::pxHtmlVideoObj = this;
}

void pxHtmlVideo2::onNewFrame(void *buffer, int w, int h, int stride, int pixel_stride)
{
  mFrameMutex.lock();
  guint8 *bits = (guint8 *)buffer;
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
  gUIThreadQueue->addTask(onRepaint, pxHtmlVideo2::pxHtmlVideoObj, nullptr);
}

pxHtmlVideo2::~pxHtmlVideo2()
{
  if (mVideoImpl) {
    delete mVideoImpl;
    mVideoImpl = NULL;
  }
}

void pxHtmlVideo2::dispose(bool /*pumpJavascript*/)
{
  mFrameMutex.lock(); // clear all frames
  while (!frames.empty()) {
    pxOffscreen *frame = frames.front();
    delete frame;
    frames.pop();
  }
  mFrameMutex.unlock();
}

void pxHtmlVideo2::draw()
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

rtError pxHtmlVideo2::attachMediaSource(rtObjectRef mediaSourceArg)
{
  MSEMediaSource *mediaSource = dynamic_cast<MSEMediaSource*>(mediaSourceArg.ptr());
  mVideoImpl->mVideoElement->setSrcObject(WebCore::MediaProvider(RefPtr(&mediaSource->getWebKitMediaSource())));
}


rtError pxHtmlVideo2::load()
{
  return RT_OK;
}

rtError pxHtmlVideo2::canPlayType(const rtString &type, bool &ret)
{
  ret = false;
  return RT_OK;
}

rtError pxHtmlVideo2::play()
{
  mVideoImpl->mVideoElement->play();
  return RT_OK;
}

rtError pxHtmlVideo2::pause()
{
  return RT_OK;
}

rtError pxHtmlVideo2::fastSeek(double time)
{
  return RT_OK;
}

void pxHtmlVideo2::onWebkitEvent(const std::string &name)
{
  mEmit.send(rtString(name.c_str()));
}

rtDefineObject(pxHtmlVideo2, pxObject);
rtDefineMethod(pxHtmlVideo2, attachMediaSource)
rtDefineProperty(pxHtmlVideo2, error)
rtDefineProperty(pxHtmlVideo2, src)
rtDefineProperty(pxHtmlVideo2, currentSrc)
rtDefineProperty(pxHtmlVideo2, crossOrigin)
rtDefineProperty(pxHtmlVideo2, networkState)
rtDefineProperty(pxHtmlVideo2, preload)
rtDefineProperty(pxHtmlVideo2, buffered)
rtDefineProperty(pxHtmlVideo2, readyState)
rtDefineProperty(pxHtmlVideo2, seeking)
rtDefineProperty(pxHtmlVideo2, currentTime)
rtDefineProperty(pxHtmlVideo2, duration)
rtDefineProperty(pxHtmlVideo2, paused)
rtDefineProperty(pxHtmlVideo2, defaultPlaybackRate)
rtDefineProperty(pxHtmlVideo2, playbackRate)
rtDefineProperty(pxHtmlVideo2, played)
rtDefineProperty(pxHtmlVideo2, seekable)
rtDefineProperty(pxHtmlVideo2, ended)
rtDefineProperty(pxHtmlVideo2, autoplay)
rtDefineProperty(pxHtmlVideo2, loop)
rtDefineProperty(pxHtmlVideo2, controls)
rtDefineProperty(pxHtmlVideo2, volume)
rtDefineProperty(pxHtmlVideo2, muted)
rtDefineProperty(pxHtmlVideo2, defaultMuted)
rtDefineProperty(pxHtmlVideo2, audioTracks)
rtDefineProperty(pxHtmlVideo2, videoTracks)
rtDefineProperty(pxHtmlVideo2, textTracks)
rtDefineMethod(pxHtmlVideo2, load)
rtDefineMethod(pxHtmlVideo2, canPlayType)
rtDefineMethod(pxHtmlVideo2, play)
rtDefineMethod(pxHtmlVideo2, pause)
rtDefineMethod(pxHtmlVideo2, fastSeek)
rtDefineMethod(pxHtmlVideo2, addListener1)
rtDefineMethod(pxHtmlVideo2, delListener1)