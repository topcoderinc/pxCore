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

#ifndef PX_HTML_VIDEO2_H
#define PX_HTML_VIDEO2_H

#include "pxScene2d.h"
#include "pxObject.h"
#include "pxContext.h"
#include <queue>

struct pxHtmlVideo2Impl;


class pxHtmlVideo2 : public pxObject {
public:
  rtDeclareObject(pxHtmlVideo2, pxObject);

  pxHtmlVideo2(pxScene2d *scene);

  virtual ~pxHtmlVideo2();

  rtMethodNoArgAndReturn("createMediaSource", createMediaSource, rtObjectRef);
  rtError createMediaSource(rtObjectRef &outMediaSource);

  rtMethodNoArgAndNoReturn("play", play);
  rtError play();

  virtual void draw();

  virtual void dispose(bool pumpJavascript);

  void onNewFrame(void *buffer, int w, int h, int stride, int pixel_stride);

  static void onRepaint(void *context, void *data);

private:

  rtMutex mFrameMutex;
  std::queue<pxOffscreen *> frames;
  pxOffscreen *previousFrame;

  pxHtmlVideo2Impl *mVideoImpl = nullptr;

public:
  static pxHtmlVideo2 *pxHtmlVideoObj; //This object
};

#endif // PX_HTML_VIDEO2_H
