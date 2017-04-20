
#include "pxRenderTexture.h"
#include "pxContext.h"

extern pxContext context;

pxRenderTexture::pxRenderTexture(pxScene2d* scene) : pxObject(scene), mWidth(0), mHeight(0) 
{

}

pxContextFramebufferRef pxRenderTexture::getCfbRef()
{
  return mCfbRef;
}

pxContextFramebufferRef pxRenderTexture::createCfbRef()
{
  if (!mCfbRef)
  {
    mCfbRef = context.createFramebuffer(mWidth, mHeight);
  }
  return mCfbRef;
}



rtDefineObject(pxRenderTexture, pxObject);
rtDefineProperty(pxRenderTexture, width);
rtDefineProperty(pxRenderTexture, height);