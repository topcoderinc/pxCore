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

// pxRectangle.h

#ifndef PX_MEDIA_SOURCE_H
#define PX_MEDIA_SOURCE_H

#include "pxScene2d.h"
#include "pxObject.h"
#include "pxContext.h"

class MSEMediaSource;

class pxMediaSource: public pxObject
{
public:
  rtDeclareObject(pxMediaSource, pxObject);

  pxMediaSource(pxScene2d* scene);
  virtual ~pxMediaSource();

  virtual void onInit();

  rtReadOnlyProperty(mediaSource, getMediaSource, rtObjectRef);

  virtual rtError getMediaSource(rtObjectRef &v) const;

  static void init();

private:
  MSEMediaSource *mMediaSource;
};

#endif // PX_VIDEO_H