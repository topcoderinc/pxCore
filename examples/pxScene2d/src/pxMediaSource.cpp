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
#include "mse/MediaSource.h"

pxMediaSource::pxMediaSource(pxScene2d *scene) :
  pxObject(scene),
  mMediaSource(new MediaSource)
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

rtDefineObject(pxMediaSource, pxObject);
rtDefineProperty(pxMediaSource, mediaSource);
