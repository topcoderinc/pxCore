/**
 * pxGraphic to render polygon, lines
 */
#ifndef PX_RENDER_TEXTURE_H
#define PX_RENDER_TEXTURE_H
#include "pxScene2d.h"
#include "pxContextFramebuffer.h"
#include "pxTexture.h"

class pxRenderTexture: public pxObject {
public:
  rtDeclareObject(pxRenderTexture, pxObject);
  rtProperty(width, width, setWidth, float);
  rtProperty(height, height, setHeight, float);


  pxRenderTexture(pxScene2d* scene);
  virtual void onInit() {mReady.send("resolve",this);}


  rtError width(float& w) const
  {
    w = mWidth;
    return RT_OK;
  }

  rtError setWidth(float w)
  {
    mWidth = w;
    return RT_OK;
  }

  rtError height(float& h) const
  {
    h = mHeight;
    return RT_OK;
  }

  rtError setHeight(float h)
  {
    mHeight = h;
    return RT_OK;
  }

  pxContextFramebufferRef getCfbRef();
  pxContextFramebufferRef createCfbRef();
  
  pxTextureRef getTexture()
  {
    static pxTextureRef nullTextureRef;
    if (mCfbRef)
    {
      return mCfbRef->getTexture();
    }
    return nullTextureRef;
  }
private:
  pxContextFramebufferRef mCfbRef;
  float mWidth;
  float mHeight;
};

#endif