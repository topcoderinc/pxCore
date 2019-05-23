#include "pxAAMPPlayer.h"
#include "pxContext.h"


#include <gst/gst.h>
#include <priv_aamp.h>
#include <main_aamp.h>
#include <StreamAbstractionAAMP.h>
#include <pxScene2d.h>


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
  AAMPGstPlayerMainLoop = NULL;
  return NULL;
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
  }
}


void pxAAMPPlayer::onInit()
{
  mReady.send("resolve", this);
  pxObject::onInit();
  updateAAMPSize();
}

rtError pxAAMPPlayer::updateUrl(rtString const &newUrl)
{
  rtLogInfo("updateUrl url = %s", newUrl.cString());
  this->mUrl = std::string(newUrl.cString());
  mSingleton->Tune(this->mUrl.c_str());
  return RT_OK;
}

pxAAMPPlayer::~pxAAMPPlayer()
{
  rtLogInfo("delete pxAAMPPlayer.");
  rtLogSetLevel(RT_LOG_WARN);
}

void pxAAMPPlayer::initAAMP()
{
  mAAMPView = new AAMPView();
  InitPlayerLoop(0, nullptr);
  mSingleton = new PlayerInstanceAAMP();
}


void pxAAMPPlayer::getGlobalPosition(float *pos)
{
  pos[0] = diffX;
  pos[1] = diffY;
  pxObject *p = this;
  while (p) {
    pos[0] += p->x();
    pos[1] += p->y();
    p = p->parent();
  }
}

void pxAAMPPlayer::updateAAMPSize()
{
  float pos[2];
  getGlobalPosition(pos);
  mAAMPView->setSize(pos[0], pos[1], mw, mh);
  rtLogError("%f %f", mw, mh);
}


void pxAAMPPlayer::dispose(bool pumpJavascript)
{
  mSingleton->Stop();
  delete mAAMPView;
  pxObject::dispose(pumpJavascript);
}

rtError pxAAMPPlayer::pause()
{
  mSingleton->SetRate(1);
  return RT_OK;
}

rtError pxAAMPPlayer::play()
{
  mSingleton->SetRate(0);
  return RT_OK;
}

rtDefineObject(pxAAMPPlayer, pxObject)
rtDefineProperty(pxAAMPPlayer, url)
rtDefineMethod(pxAAMPPlayer, pause)
rtDefineMethod(pxAAMPPlayer, play)