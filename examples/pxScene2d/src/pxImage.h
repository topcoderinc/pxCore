/*

 pxCore Copyright 2005-2017 John Robinson

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

// pxImage.h

#ifndef PX_IMAGE_H
#define PX_IMAGE_H

#include "pxContext.h"
#include "rtMutex.h"
#include "pxTexture.h"
//#include "pxTextureCacheObject.h"
#include "pxResource.h"
#include "pxRenderTexture.h"

class pxImage: public pxObject, pxResourceListener 
{
public:
  rtDeclareObject(pxImage, pxObject);
  rtProperty(url, url, setUrl, rtString);
  rtProperty(stretchX, stretchX, setStretchX, int32_t);
  rtProperty(stretchY, stretchY, setStretchY, int32_t);
  rtProperty(resource, resource, setResource, rtObjectRef);
  rtProperty(texture, texture, setTexture, rtObjectRef);
  rtProperty(uvs, uvs, setUvs, rtObjectRef);

  rtProperty(vertices, vertices, setVertices, rtObjectRef);
  rtProperty(type, type, setType, rtString);

  pxImage(pxScene2d* scene) : pxObject(scene), mStretchX(pxConstantsStretch::NONE), 
  mStretchY(pxConstantsStretch::NONE), mUvs(NULL), mVertices(NULL), mUvsLength(0),  
    imageLoaded(false), mListenerAdded(false)
  { 
    mw = -1;
    mh = -1;
    mResource = pxImageManager::getImage("");
  }

  virtual ~pxImage();

  virtual void update(double t) { pxObject::update(t);}
  virtual void onInit();
  virtual void sendPromise();
  virtual void createNewPromise() { rtLogDebug("pxImage ignoring createNewPromise\n"); }
  
  rtError url(rtString& s) const;
  rtError setUrl(const char* s);
  
  rtError stretchX(int32_t& v) const { v = (int32_t)mStretchX; return RT_OK; }
  rtError setStretchX(int32_t v);

  rtError stretchY(int32_t& v) const { v = (int32_t)mStretchY; return RT_OK; }
  rtError setStretchY(int32_t v);
  
  rtError resource(rtObjectRef& o) const { /*rtLogDebug("!!!!!!!!!!!!!!!!!!!!!!!pxImage getResource\n");*/o = mResource; return RT_OK; }
  rtError setResource(rtObjectRef o);

  rtError texture(rtObjectRef& t) const 
  {
    t = mTexture;
    return RT_OK;
  }

  rtError setTexture(rtObjectRef texture)
  {
    mTexture = texture;
    return RT_OK;
  }
  
  rtError uvs(rtObjectRef& /*uvs*/) const { return RT_OK; }
  rtError setUvs(rtObjectRef uvs) 
  {
    if (!uvs) return RT_ERROR;
    uint32_t len = uvs.get<uint32_t>("length"); 
    if (len < 8)
    {
      rtLogError("uvs length must be greater than 8.");
      return RT_ERROR;
    }
    mUvsLength = len/2;
    mUvs = copyJSArray(uvs, mUvs);
    if (!mUvs)
    {
      return RT_ERROR;
    }
    return RT_OK;
  }

  rtError vertices(rtObjectRef& /*vertices*/) const { return RT_OK; }
  rtError setVertices(rtObjectRef vertices)
  {
    mVertices = copyJSArray(vertices, mVertices);
    if (!mVertices)
    {
      return RT_ERROR;
    }
    return RT_OK;
  }

  rtError type(rtString & /*type*/) const { return RT_OK; }
  rtError setType(rtString t) 
  {
    strcpy(mType, t.cString());
    return RT_OK;
  }

  

  template<typename T> T* copyJSArray(rtObjectRef source, T* op)
  {
    if (!op)
    {
      free(op);
    }
    if (!source)
    {
      return NULL;
    }

    uint32_t len = source.get<uint32_t>("length");
    T* dest = (T*)malloc(sizeof(T) * len);
    for (uint32_t i = 0; i < len; i++) 
    {
      dest[i] = source.get<T>(i);
    }
    return dest;
  }

  virtual void resourceReady(rtString readyResolution);
  //virtual bool onTextureReady(pxTextureCacheObject* textureCacheObject) {return true;}
  // !CLF: To Do: These names are terrible... find better ones!
  virtual float getOnscreenWidth();
  virtual float getOnscreenHeight();
  virtual void dispose();
  void checkStretchX();
  void checkStretchY();

  pxRenderTexture * getRenderTexture()
  {
    if (mTexture.getPtr() != NULL)
    {
      pxObject* t = (pxObject*) mTexture.getPtr();
      pxRenderTexture* renderTexture = dynamic_cast<pxRenderTexture*>(t);
      return renderTexture;
    }
    return NULL;
  }

protected:
  virtual void draw();
  
  void drawMesh();
  void drawWithImage();
  void drawWithTexture(pxTextureRef texture);

  void loadImage(rtString Url);
  inline rtImageResource* getImageResource() const { return (rtImageResource*)mResource.getPtr(); }

  pxConstantsStretch::constants mStretchX;
  pxConstantsStretch::constants mStretchY;
  rtObjectRef mResource;
  rtObjectRef mTexture;
  
  float* mUvs;
  float* mVertices;
  uint32_t mUvsLength;
  char mType[32]; 

  bool imageLoaded;
  bool mListenerAdded;
};

#endif
