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

// pxText.h

#ifndef PX_TEXT_H
#define PX_TEXT_H

// TODO it would be nice to push this back into implemention
#include <ft2build.h>
#include FT_FREETYPE_H

#include "rtString.h"
#include "rtRef.h"
#include "pxScene2d.h"
#include "pxFont.h"
#include "pxResource.h"

class pxText: public pxObject, pxResourceListener 
{
public:
  rtDeclareObject(pxText, pxObject);
  rtProperty(text, text, setText, rtString);
  rtProperty(textColor, textColor, setTextColor, uint32_t);
  rtProperty(gradientColor,gradientColor, setGradientColor, uint32_t);
  rtProperty(pixelSize, pixelSize, setPixelSize, uint32_t);
  rtProperty(fontUrl, fontUrl, setFontUrl, rtString);  
  rtProperty(font, font, setFont, rtObjectRef);

  rtProperty(italic, italic, setItalic, bool);
  rtProperty(bold, bold, setBold, bool);
  rtProperty(strokeWidth, strokeWidth,setStrokeWidth, float);
  rtProperty(strokeColor, strokeColor,setStrokeColor, uint32_t);
  rtProperty(dropShadow, dropShadow, setDropShadow,bool);
  rtProperty(dropShadowColor, dropShadowColor,setDropShadowColor, uint32_t);
  rtProperty(dropShadowOffsetX, dropShadowOffsetX, setDropShadowOffsetX,float);
  rtProperty(dropShadowOffsetY, dropShadowOffsetY, setDropShadowOffsetY, float);
  rtProperty(dropShadowBlur, dropShadowBlur,setDropShadowBlur,float);

  pxText(pxScene2d* scene);
  virtual ~pxText();
  rtError text(rtString& s) const;
  virtual rtError setText(const char* text);

  rtError uintToFloatArray(uint32_t c , float * arr){
    arr[0] = (float)((c>>24)&0xff)/255.0f;
    arr[1] = (float)((c>>16)&0xff)/255.0f;
    arr[2] = (float)((c>>8)&0xff)/255.0f;
    arr[3] = (float)((c>>0)&0xff)/255.0f;
    return RT_OK;
  }

  rtError textColor(uint32_t& /*c*/) const { return RT_OK; }
  rtError setTextColor(uint32_t c) { return uintToFloatArray(c,mTextColor); }


  rtError strokeWidth(float & /*w*/) const { return RT_OK; }
  virtual rtError setStrokeWidth(float w);

  rtError strokeColor(uint32_t& /*c*/) const { return RT_OK; }
  rtError setStrokeColor(uint32_t c) { return uintToFloatArray(c,mStrokeColor); }


  rtError dropShadowColor(uint32_t& /*c*/) const { return RT_OK; }
  rtError setDropShadowColor(uint32_t c) { return uintToFloatArray(c,mDropShadowColor); }

  rtError gradientColor(uint32_t& /*c*/) const { return RT_OK; }
  rtError setGradientColor(uint32_t c) { return uintToFloatArray(c,mGradientColor); }


  rtError italic(bool & /*b*/) const { return RT_OK; }
  virtual rtError setItalic(bool b);

  rtError bold(bool & /*b*/) const { return RT_OK; }
  virtual rtError setBold(bool b);


  rtError dropShadow(bool & /*b*/) const { return RT_OK; }
  virtual rtError setDropShadow(bool b);

  rtError dropShadowOffsetX(float & /*x*/) const { return RT_OK; }
  virtual rtError setDropShadowOffsetX(float x);

  rtError dropShadowOffsetY(float & /*y*/) const { return RT_OK; }
  virtual rtError setDropShadowOffsetY(float y);

  rtError dropShadowBlur(float & /*b*/) const { return RT_OK; }
  virtual rtError setDropShadowBlur(float b);

  void updateShadowParams();

  rtError fontUrl(rtString& v) const { getFontResource()->url(v); return RT_OK; }
  virtual rtError setFontUrl(const char* s);

  rtError pixelSize(uint32_t& v) const { v = mPixelSize; return RT_OK; }
  virtual rtError setPixelSize(uint32_t v);
  
  rtError font(rtObjectRef& o) const { o = mFont; return RT_OK; }
  virtual rtError setFont(rtObjectRef o);
  
  virtual void update(double t);
  virtual void onInit();
  
  virtual rtError Set(const char* name, const rtValue* value)
  {
    //rtLogInfo("pxText::Set %s\n",name);
#if 1
    mDirty = mDirty || (!strcmp(name,"w") ||
              !strcmp(name,"h") ||
              !strcmp(name,"text") ||
              !strcmp(name,"pixelSize") ||
              !strcmp(name,"fontUrl") ||
              !strcmp(name,"textColor"));
#else
    mDirty = true;
#endif
    rtError e = pxObject::Set(name, value);
    

    return e;
  }

  virtual void resourceReady(rtString readyResolution);
  virtual void sendPromise();
  virtual float getOnscreenWidth();
  virtual float getOnscreenHeight();

 protected:
  virtual void draw();
  // !CLF ToDo: Could mFont.send(...) be used in places where mFont is needed, instead
  // of this getFontResource?
  inline pxFont* getFontResource() const { return (pxFont*)mFont.getPtr(); }  
  
  rtString mText;
// TODO should we just use a font object instead of Urls
  bool mFontLoaded;
//  rtString mFontUrl;

  rtObjectRef mFont;
  
  float mTextColor[4];
  float mStrokeColor[4];
  float mDropShadowColor[4];
  float mGradientColor[4];

  bool isItalic;
  bool isBold;
  float mStrokeWidth;
  bool isDropShadow;
  float mDropShadowOffsetX;
  float mDropShadowOffsetY;
  float mDropShadowBlur;

  uint32_t mPixelSize;
  bool mDirty;
  pxContextFramebufferRef mCached;
  rtFileDownloadRequest* mFontDownloadRequest;
  
  virtual float getFBOWidth() { return mw; }
  virtual float getFBOHeight() { return mh; }
  bool mListenerAdded;
};

#endif
