#include "pxAAMPPlayer.h"
#include "pxContext.h"
#include "SourceBuffer.h"
#include "pxScene2d.h"


#include <gst/gst.h>
#include <priv_aamp.h>
#include <main_aamp.h>
#include <StreamAbstractionAAMP.h>
#include <pxScene2d.h>


extern pxContext context;

/**
 * static things wont release unit app exist
 */
static PlayerInstanceAAMP *mSingleton;
static GMainLoop *AAMPGstPlayerMainLoop = nullptr;

static void *AAMPGstPlayer_StreamThread(void *arg);

static bool initialized = false;
GThread *aampMainLoopThread = nullptr;


/**
 * @brief Thread to run mainloop (for standalone mode)
 * @param[in] arg user_data
 * @retval void pointer
 */
static void *AAMPGstPlayer_StreamThread(void *arg)
{
  if (AAMPGstPlayerMainLoop) {
    g_main_loop_run(AAMPGstPlayerMainLoop); // blocks
    logprintf("AAMPGstPlayer_StreamThread: exited main event loop\n");
  }
  g_main_loop_unref(AAMPGstPlayerMainLoop);
  AAMPGstPlayerMainLoop = nullptr;
  return nullptr;
}

/**
 * @brief To initialize Gstreamer and start mainloop (for standalone mode)
 * @param[in] argc number of arguments
 * @param[in] argv array of arguments
 */
void InitPlayerLoop(int argc, char **argv)
{
  if (!initialized) {
    initialized = true;
    gst_init(&argc, &argv);
    AAMPGstPlayerMainLoop = g_main_loop_new(nullptr, FALSE);
    aampMainLoopThread = g_thread_new("AAMPGstPlayerLoop", &AAMPGstPlayer_StreamThread, nullptr);
    mSingleton = new PlayerInstanceAAMP();
    mSingleton->aamp->mUseImageSink = true;
  }
}

extern pxContext context;

void pxAAMPPlayer::onInit()
{
  mReady.send("resolve", this);
  pxObject::onInit();
  mMediaSource->load(mSrc.cString());
}


pxAAMPPlayer::~pxAAMPPlayer()
{
  rtLogWarn("delete pxAAMPPlayer.");
}

pxAAMPPlayer::pxAAMPPlayer(pxScene2d *scene) : pxObject(scene), mMediaSource(new MediaSource),
                                               mMuted(false), mRenderFrameId(0), mTimeTik(0), mPreviousFrameTime(-1.0)
{
  prepareAAMP();
  InitPlayerLoop(0, nullptr);
  mRenderFrameId = 0;
  mOffscreen = new pxOffscreen();
  mMediaSource->setInstanceAamp(mSingleton);
}

rtError pxAAMPPlayer::getSrc(rtString &v) const
{
  v = mSrc;
  return RT_OK;
}

rtError pxAAMPPlayer::updateSrc(rtString const &newSrc)
{
  mMediaSource->addSourceBuffer(new SourceBuffer);
  mSrc = newSrc;
  return RT_OK;
}


void pxAAMPPlayer::dispose(bool pumpJavascript)
{
  mSingleton->Stop();
  if (mSingleton->aamp->mFrameData) {
    free(mSingleton->aamp->mFrameData);
    mSingleton->aamp->mFrameData = nullptr;
    mSingleton->aamp->mFrameId = 0;
  }
  pxObject::dispose(pumpJavascript);
}

void pxAAMPPlayer::draw()
{
  // whole spark only have one aamp player, so here use mSingleton instead of mMediaSource->getInstanceAamp()
  if (mSingleton->aamp->mFrameData) {
    if (mSingleton->aamp->mIsCopyingFrame) {}
    else if (mRenderFrameId == mSingleton->aamp->mFrameId) {
      // bad network (waiting network), or video fps too lower(waiting new frame)
    } else {
      mRenderFrameId = mSingleton->aamp->mFrameId;
      int w = mSingleton->aamp->mFrameWidth;
      int h = mSingleton->aamp->mFrameHeight;
      uint8_t *bits = mSingleton->aamp->mFrameData;
      mOffscreen->init(w, h);  // video frame size may changed when netword changed, so need reset size

      // TODO here need more effective method to copy frame data to offscreen
      for (int y = 0; y < h; y++) {
        pxPixel *d = mOffscreen->scanline(y);
        for (int x = 0; x < w; x++) {
          int index = y * w + x;
          *d = pxPixel(bits[index * 4], bits[index * 4 + 1], bits[index * 4 + 2]);
          d++;
        }
      }
    }
    mTextureRef = context.createTexture(*mOffscreen);
    context.drawImage(0, 0, mw, mh, mTextureRef, nullptr, false);
  }

  mScene->mDirty = true;
  markDirty(); // always try to redraw
}

rtError pxAAMPPlayer::pause()
{
  mMediaSource->getInstanceAamp()->SetRate(0);
  return RT_OK;
}

rtError pxAAMPPlayer::play()
{
  mMediaSource->getInstanceAamp()->SetRate(1);
  return RT_OK;
}

rtError pxAAMPPlayer::fastSeek(double time)
{
  mMediaSource->getInstanceAamp()->Seek(time);
  return RT_OK;
}

rtError pxAAMPPlayer::getMediaSource(rtObjectRef &v) const
{
  v = mMediaSource;
  return RT_OK;
}

rtError pxAAMPPlayer::getDuration(double &v) const
{
  v = mMediaSource->getInstanceAamp()->GetPlaybackDuration();
  return RT_OK;
}

rtError pxAAMPPlayer::getCurrentTime(double &v) const
{
  v = mMediaSource->getInstanceAamp()->GetPlaybackPosition();
  return RT_OK;
}

rtError pxAAMPPlayer::getDefaultMuted(bool &v) const
{
  v = mMuted;
  return RT_OK;
}

rtError pxAAMPPlayer::setDefaultMuted(bool const &v)
{
  mMediaSource->getInstanceAamp()->SetVideoMute(v);
  mMuted = v;
  return RT_OK;
}

void pxAAMPPlayer::update(double t, bool updateChildren)
{
  if (mPreviousFrameTime <= 0) {
    mPreviousFrameTime = t;
  }

  mTimeTik += (t - mPreviousFrameTime);

  if (mTimeTik > 0.5) {
    mEmit.send("update");
    mTimeTik = 0;
  }

  mPreviousFrameTime = t;
  pxObject::update(t, updateChildren);
}


rtDefineObject(pxAAMPPlayer, pxObject)


rtDefineProperty(pxAAMPPlayer, src)
rtDefineProperty(pxAAMPPlayer, mediaSource)
rtDefineProperty(pxAAMPPlayer, currentSrc)
rtDefineProperty(pxAAMPPlayer, currentTime)
rtDefineProperty(pxAAMPPlayer, duration)
rtDefineProperty(pxAAMPPlayer, defaultMuted)
rtDefineMethod(pxAAMPPlayer, pause)
rtDefineMethod(pxAAMPPlayer, play)
rtDefineMethod(pxAAMPPlayer, fastSeek)