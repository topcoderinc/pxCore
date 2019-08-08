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

#include "pxMediaSource.h"
#include "mse/MSEMediaSource.h"

#ifdef ENABLE_SPARK_VIDEO
#include "WebCore/config.h"
#include <wtf/RunLoop.h>
#include <WebCore/LogInitialization.h>
#include <WebCore/page/ProcessWarming.h>
#include <WebCore/platform/Timer.h>
#include <wtf/Threading.h>
#include <gst/gst.h>
#endif

pxMediaSource::pxMediaSource(pxScene2d *scene) :
  pxObject(scene),
  mMediaSource(new MSEMediaSource)
{
}

pxMediaSource::~pxMediaSource()
{
  delete mMediaSource;
  mMediaSource = NULL;
}

void pxMediaSource::onInit()
{
  pxObject::onInit();
}

rtError pxMediaSource::getMediaSource(rtObjectRef &v) const
{
  v = mMediaSource;
  return RT_OK;
}

#ifdef ENABLE_SPARK_VIDEO
static bool initialized = false;
static void *runLoopThread(void *)
{

  WTF::RunLoop::run();
  return 0;
}
#endif

void pxMediaSource::init()
{
#ifdef ENABLE_SPARK_VIDEO
  if (!initialized) {
    initialized = true;
    //GstPlayerMainLoop = g_main_loop_new(nullptr, FALSE);
    g_thread_new("GstPlayerLoop", &runLoopThread, nullptr);
  }
#endif
}


rtDefineObject(pxMediaSource, pxObject);
rtDefineProperty(pxMediaSource, mediaSource);
