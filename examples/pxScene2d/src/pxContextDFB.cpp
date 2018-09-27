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

// pxContextDFB.cpp

#include "rtCore.h"
#include "rtLog.h"
#include "rtThreadTask.h"
#include "rtThreadPool.h"
#include "rtThreadQueue.h"
#include "rtMutex.h"
#include "rtScript.h"
#include "rtSettings.h"

#include "pxContext.h"
#include "pxUtil.h"
#include <algorithm>
#include <ctime>
#include <cstdlib>

#include <directfb.h>

////////////////////////////////////////////////////////////////
//
// Debug macros...

// NOTE:  Comment out these defines for 'normal' operation.
//
// #define DEBUG_SKIP_RECT
// #define DEBUG_SKIP_IMAGE
// #define DEBUG_SKIP_IMAGE9
// #define DEBUG_SKIP_MATRIX    // skip setMatrix()
// #define DEBUG_SKIP_APPLY     // skip applyMatrix()

// #define DEBUG_SKIP_DIAG_RECT
// #define DEBUG_SKIP_DIAG_LINE

// #define DEBUG_SKIP_FLIPPING
// #define DEBUG_SKIP_BLIT
// #define DEBUG_SKIP_ALPHA_BLEND
// #define DEBUG_SKIP_ALPHA_BLEND_FUNC
// #define DEBUG_SKIP_PORTER_DUFF

// #define DEBUG_SKIP_CLEAR  // pseudo color

#define PREFERRED_MEMORY     DSCAPS_VIDEOONLY   // DSCAPS_VIDEOONLY  DSCAPS_SYSTEMONLY

#define F2F_SHIFT (0x10000)   //  Floating-Point to Fixed-Point shift.

////////////////////////////////////////////////////////////////
//
// Debug Statistics
#ifdef USE_RENDER_STATS
  extern uint32_t gDrawCalls;
  extern uint32_t gTexBindCalls;
  extern uint32_t gFboBindCalls;

  #define TRACK_DRAW_CALLS()   { gDrawCalls++;    }
  #define TRACK_TEX_CALLS()    { gTexBindCalls++; }
  #define TRACK_FBO_CALLS()    { gFboBindCalls++; }
#else
  #define TRACK_DRAW_CALLS()
  #define TRACK_TEX_CALLS()
  #define TRACK_FBO_CALLS()
#endif

rtThreadQueue* gUIThreadQueue = new rtThreadQueue();

////////////////////////////////////////////////////////////////

// DSPF_ABGR == Ubuntu
//
// DSPF_ARGB == RNG150

#ifdef ENABLE_DFB_GENERIC
IDirectFB                *dfb            = NULL;
IDirectFBSurface         *dfbSurface     = NULL;
DFBSurfacePixelFormat     dfbPixelformat = DSPF_ABGR;

bool needsFlip = true;

IDirectFB                *outsideDfb        = NULL;
IDirectFBSurface         *outsideDfbSurface = NULL;

pxContext context; // needed here...  BUT - ".so" load fails if removed.  Must be referred to *somewhere*

#else

extern pxContext                context;

extern IDirectFB                *dfb;
extern IDirectFBSurface         *dfbSurface;
extern DFBSurfacePixelFormat     dfbPixelformat;

extern bool needsFlip;

#endif //ENABLE_DFB_GENERIC

#ifdef RUNINMAIN
extern rtScript                   script;
#else
extern uv_async_t 				gcTrigger;
#endif

#ifdef DEBUG
// Use div0 to trigger debugger to break...

#define DFB_CHECK(x...)              \
{                                    \
   DFBResult err = x;                \
                                     \
   if (err != DFB_OK)                \
   {                                 \
      rtLogError( " DFB died... " ); \
      int n = 0;  int m = 2/n;\
         DirectFBErrorFatal( #x, err ); \
   }                                 \
}

static void ReportSurfaceInfo(char* desc, IDirectFBSurface* surf); // DEBUG

//static std::string DFBAccelerationMask2str(DFBAccelerationMask m); //fwd
//static        void DFBAccelerationTest(IDirectFBSurface  *dst, IDirectFBSurface  *src); //fwd

char*  pix2str(DFBSurfacePixelFormat   fmt); // DEBUG
char* caps2str(DFBSurfaceCapabilities caps); // DEBUG

#else

#define DFB_CHECK(x...)  (x)

#endif

//====================================================================================================================================================================================

//#warning TODO:  Use  pxContextSurfaceNativeDesc  within   pxFBOTexture ??

pxContextSurfaceNativeDesc  defaultContextSurface;
pxContextSurfaceNativeDesc* currentContextSurface = &defaultContextSurface;

pxContextFramebufferRef defaultFramebuffer(new pxContextFramebuffer());
pxContextFramebufferRef currentFramebuffer = defaultFramebuffer;

// TODO get rid of this global crap

static int gResW, gResH;
static pxMatrix4f gMatrix;
static float gAlpha = 1.0;
uint32_t gRenderTick = 0;
std::vector<pxTexture*> textureList;
#ifdef ENABLE_BACKGROUND_TEXTURE_CREATION
rtMutex contextLock;
#endif //ENABLE_BACKGROUND_TEXTURE_CREATION

pxError lockContext()
{
#ifdef ENABLE_BACKGROUND_TEXTURE_CREATION
  contextLock.lock();
#endif //ENABLE_BACKGROUND_TEXTURE_CREATION
  return PX_OK;
}

pxError unlockContext()
{
#ifdef ENABLE_BACKGROUND_TEXTURE_CREATION
  contextLock.unlock();
#endif //ENABLE_BACKGROUND_TEXTURE_CREATION
  return PX_OK;
}

pxError addToTextureList(pxTexture* texture)
{
  if(texture == NULL)
  {
    //rtLogError("addToTextureList() - Passed NULL - ignoring.");
    return PX_OK; // skip
  }

  textureList.push_back(texture);
  return PX_OK;
}

pxError removeFromTextureList(pxTexture* texture)
{
  for(std::vector<pxTexture*>::iterator it = textureList.begin(); it != textureList.end(); ++it)
  {
    if ((*it) == texture)
    {
      textureList.erase(it);
      return PX_OK;
    }
  }
  return PX_OK;
}

pxError ejectNotRecentlyUsedTextureMemory(int64_t bytesNeeded, uint32_t maxAge=5)
{
  rtLogDebug("attempting to eject %d bytes of texture memory with max age %u", bytesNeeded, maxAge);
#if !defined(DISABLE_TEXTURE_EJECTION)
  int numberEjected = 0;
  int64_t beforeTextureMemoryUsage = context.currentTextureMemoryUsageInBytes();

  std::random_shuffle(textureList.begin(), textureList.end());
  for(std::vector<pxTexture*>::iterator it = textureList.begin(); it != textureList.end(); ++it)
  {
    pxTexture* texture = (*it);
    uint32_t lastRenderTickAge = gRenderTick - texture->lastRenderTick();
    if (lastRenderTickAge >= maxAge)
    {
      numberEjected++;
      texture->unloadTextureData();
      int64_t currentTextureMemory = context.currentTextureMemoryUsageInBytes();
      if ((beforeTextureMemoryUsage - currentTextureMemory) > bytesNeeded)
      {
        break;
      }
    }
  }

  if (numberEjected > 0)
  {
    int64_t afterTextureMemoryUsage = context.currentTextureMemoryUsageInBytes();
    rtLogWarn("%d textures have been ejected and %d bytes of texture memory has been freed",
        numberEjected, (beforeTextureMemoryUsage - afterTextureMemoryUsage));
  }
#endif //!DISABLE_TEXTURE_EJECTION
  return PX_OK;
}


int64_t getSurfaceByteSize(IDirectFBSurface* surf)
{
  if(surf)
  {
     int w = 0, h = 0;
     surf->GetSize(surf, &w, &h);

     DFBSurfacePixelFormat	 fmt;
     surf->GetPixelFormat(surf, &fmt);
     
     int bytesPerPixel = (fmt == DSPF_A8) ? 1 : 4; // Either DSPF_A8 / DSPF_ABGR / DSPF_ARGB
     
     return (w * h * bytesPerPixel);
  }
  
  return 0;
}

static inline void applyMatrix(IDirectFBSurface  *surface, const float *mm);

//====================================================================================================================================================================================

// more globals :(
static IDirectFBSurface  *boundTextureMask = NULL;
static IDirectFBSurface  *boundTexture     = NULL;
static IDirectFBSurface  *boundFramebuffer = NULL; // default

inline void premultiply(float* d, const float* s)
{
   d[0] = s[0] * s[3];
   d[1] = s[1] * s[3];
   d[2] = s[2] * s[3];
   d[3] = s[3];
}

//====================================================================================================================================================================================

class pxFBOTexture : public pxTexture
{
public:
  pxFBOTexture() : mWidth(0), mHeight(0), mOffscreen(), mTexture(NULL)
  {
    mTextureType = PX_TEXTURE_FRAME_BUFFER;
  }

  ~pxFBOTexture()  { deleteTexture(); }

  void createTexture(int w, int h)
  {
    //rtLogDebug("############# this: %p >>  %s  WxH: %d x %d \n", this, __PRETTY_FUNCTION__, w,h);

    if (mTexture != NULL)
    {
        deleteTexture();
    }

    if(w <= 0 || h <= 0)
    {
        rtLogError("\nERROR:  %s(%d, %d) - INVALID",__PRETTY_FUNCTION__, w,h);
        return;
    }

//    rtLogDebug("############# this: %p >>  %s(%d, %d) \n", this, __PRETTY_FUNCTION__, w, h);

    mWidth  = w;
    mHeight = h;

    int64_t bytes = getSurfaceByteSize(mTexture);
    context.adjustCurrentTextureMemorySize(bytes);  // CREATE: pxFBOTexture

    mOffscreen.init(mWidth, mHeight);

    createSurface(mOffscreen); // surface is framebuffer

    DFB_CHECK(mTexture->Clear(mTexture, 0x00, 0x00, 0x00, 0x00 ) );  // TRANSPARENT
    //DFB_CHECK(mTexture->Clear(mTexture, 0x00, 0xFF, 0x00, 0xFF ) );  // DEBUG - GREEN !!!!

    //rtLogDebug("############# this: %p >>  %s - CLEAR_GREEN !!!!\n", this, __PRETTY_FUNCTION__);
  }

  pxError resizeTexture(int w, int h)
  {
    rtLogDebug("############# this: %p >>  %s  WxH: %d, %d\n", this, __PRETTY_FUNCTION__, w, h);

    if (mWidth != w || mHeight != h || !mTexture)
    {
        createTexture(w, h);

        boundFramebuffer = mTexture;
        return PX_OK;
    }

    // TODO: Create new texture and "StretchBlit" onto it to scale it...

    //TODO - remove commented out section
    /*glBindTexture(GL_TEXTURE_2D, mTextureId);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 w, h, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, PX_TEXTURE_MIN_FILTER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, PX_TEXTURE_MAG_FILTER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/

    boundFramebuffer = mTexture;

    return PX_OK;
  }

  virtual pxError deleteTexture()
  {
    rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",mTexture, __PRETTY_FUNCTION__);

    if(mTexture)
    {
      // if(currentFramebuffer == this)
      // {
      //   // Is this sufficient ? or Dangerous ?
      //   currentFramebuffer = NULL;
      //   boundTexture = NULL;
      // }

      int64_t bytes = getSurfaceByteSize(mTexture);
      context.adjustCurrentTextureMemorySize(-1 * bytes);

      mTexture->Release(mTexture);
      mTexture = NULL;
    }

    return PX_OK;
  }

  virtual unsigned int getNativeId()
  {
    return mTextureId;
  }

  virtual pxError prepareForRendering()
  {
    rtLogDebug("############# this: (virtual) >>  %s  ENTER\n", __PRETTY_FUNCTION__);

    boundFramebuffer = mTexture;
    boundTexture     = mTexture;

    gResW = mWidth;
    gResH = mHeight;

    return PX_OK;
  }

// TODO get rid of pxError
// TODO get rid of bindTexture() and bindTextureAsMask()
  virtual pxError bindGLTexture(int /*tLoc*/)
  {
    if (mTexture == NULL)
    {
        rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
        return PX_NOTINITIALIZED;
    }

    boundTexture = mTexture;  TRACK_TEX_CALLS();

    return PX_OK;
  }

  virtual pxError bindGLTextureAsMask(int /*tLoc*/)
  {
    if (!mTexture)
    {
        rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
        return PX_NOTINITIALIZED;
    }

    boundTextureMask = mTexture;  TRACK_TEX_CALLS();

    return PX_OK;
  }

#if 1 // Do we need this?  maybe for some debugging use case??
  virtual pxError getOffscreen(pxOffscreen& /* o */)
  {
    // TODO
    return PX_FAIL;
  }
#endif

  virtual int width()   { return mWidth;  }
  virtual int height()  { return mHeight; }

//   void                    setSurface(IDirectFBSurface* s)     { mTexture  = s; };

   void*       getSurface()     { return (void*)mTexture; };
//   DFBVertex*              getVetricies()   { return &v[0];   };
//   DFBSurfaceDescription   getDescription() { return dsc;     };

  DFBVertex v[4];  //quad


private:
  int                     mWidth;
  int                     mHeight;
  int                     mTextureId;

  pxOffscreen             mOffscreen;

  IDirectFBSurface       *mTexture;

  DFBSurfaceDescription   dsc;

private:

  pxError createSurface(pxOffscreen& o)
  {
    if(mTexture)
    {
        //rtLogError("###    NOT EMPTY !!!   >>>     pxFBOTexture::createSurface()  ");

        deleteTexture();
    }
    
    mWidth  = o.width();
    mHeight = o.height();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Create setting for 'tmp'
    //
    dsc.width                 = o.width();
    dsc.height                = o.height();
    dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT| DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT);
    dsc.pixelformat           = dfbPixelformat;
    dsc.preallocated[0].data  = o.base();      // Buffer is your data
    dsc.preallocated[0].pitch = o.width() * 4;
    dsc.preallocated[1].data  = NULL;          // Not using a back buffer
    dsc.preallocated[1].pitch = 0;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    IDirectFBSurface  *tmp;

    DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &tmp ) );

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Create setting for 'mTexture'
    //
    dsc.width                 = o.width();
    dsc.height                = o.height();
    dsc.caps                  = DFBSurfaceCapabilities(PREFERRED_MEMORY); // Use Video Memory //   DSCAPS_PREMULTIPLIED
    dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
    dsc.pixelformat           = dfbPixelformat;

    DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &mTexture ) );

    mTexture->Clear(mTexture, 0, 0, 0, 0); // Transparent

#ifndef DEBUG_SKIP_BLIT
    DFB_CHECK(mTexture->Blit(mTexture, tmp, NULL, 0,0)); TRACK_DRAW_CALLS(); // blit copy to surface
#else
    #warning " 'DEBUG_SKIP_BLIT' is Enabled"
#endif

    DFB_CHECK(mTexture->SetRenderOptions(mTexture,
            DFBSurfaceRenderOptions(DSRO_MATRIX /*| DSRO_ANTIALIAS*/) ));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // Dispose of 'tmp'...
    tmp->Release(tmp);
    tmp = NULL;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    return PX_OK;
  }

};// CLASS - pxFBOTexture

//====================================================================================================================================================================================

class pxTextureNone : public pxTexture
{
public:
  pxTextureNone() {}

  virtual int width()                                 { return 0; }
  virtual int height()                                { return 0; }
  virtual pxError deleteTexture()                     { return PX_FAIL; }
  virtual pxError resizeTexture(int /*w*/, int /*h*/) { return PX_FAIL; }
  virtual pxError getOffscreen(pxOffscreen& /*o*/)    { return PX_FAIL; }
  virtual pxError bindGLTexture(int /*tLoc*/)         { return PX_FAIL; }
  virtual pxError bindGLTextureAsMask(int /*mLoc*/)   { return PX_FAIL; }

};// CLASS - pxTextureNone

//====================================================================================================================================================================================

class pxTextureOffscreen;
typedef rtRef<pxTextureOffscreen> pxTextureOffscreenRef;

struct DecodeImageData
{
  DecodeImageData(pxTextureOffscreenRef t) : textureOffscreen(t)
  {
  }
  pxTextureOffscreenRef textureOffscreen;

};

void onDecodeComplete(void* context, void* data);
void decodeTextureData(void* data);
void onOffscreenCleanupComplete(void* context, void*);
void cleanupOffscreen(void* data);

class pxTextureOffscreen : public pxTexture
{
public:
  pxTextureOffscreen() : mOffscreen(), mInitialized(false), mWidth(0), mHeight(0),
                         mTextureUploaded(false), mTexture(NULL),
                         mTextureDataAvailable(false), mLoadTextureRequested(false), mOffscreenMutex(),
                         mFreeOffscreenDataRequested(false), mCompressedData(NULL), mCompressedDataSize(0)
  {
    mTextureType = PX_TEXTURE_OFFSCREEN;
    addToTextureList(this);
  }

  pxTextureOffscreen(pxOffscreen& o, const char *compressedData = NULL, size_t compressedDataSize = 0) 
                                     : mOffscreen(), mInitialized(false), mWidth(0), mHeight(0),
                                       mTextureUploaded(false),  mTexture(NULL),
                                       mTextureDataAvailable(false), mLoadTextureRequested(false), mOffscreenMutex(),
                                       mFreeOffscreenDataRequested(false), mCompressedData(NULL), mCompressedDataSize(0)
  {
    mTextureType = PX_TEXTURE_OFFSCREEN;
    setCompressedData(compressedData, compressedDataSize);
    createTexture(o);
    addToTextureList(this);
  }

~pxTextureOffscreen() { deleteTexture(); removeFromTextureList(this);};

  virtual pxError createTexture(pxOffscreen& o)
  {
    mOffscreenMutex.lock();
    mOffscreen.init(o.width(), o.height());

//#ifndef DEBUG_SKIP_BLIT
    o.blit(mOffscreen);
//#endif


    // premultiply
#if 1
    for (int y = 0; y < mOffscreen.height(); y++)
    {
      pxPixel* d  = mOffscreen.scanline(y);
      pxPixel* de = d + mOffscreen.width();
      while (d < de)
      {
        d->r = (d->r * d->a)/255;
        d->g = (d->g * d->a)/255;
        d->b = (d->b * d->a)/255;
        d++;
      }
    }
#endif

    mWidth  = mOffscreen.width();
    mHeight = mOffscreen.height();

    createSurface(o);

    mFreeOffscreenDataRequested = false;
    mOffscreenMutex.unlock();

    mLoadTextureRequested = false;
    mInitialized = true;

    return PX_OK;
  }

  virtual pxError deleteTexture()
  {
    rtLogDebug("pxTextureOffscreen::deleteTexture()");

    unloadTextureData();

    freeCompressedData();
    mInitialized = false;
    return PX_OK;
  }

  virtual pxError loadTextureData()
  {
    if (!mLoadTextureRequested && mTextureDataAvailable)
    {
      rtThreadPool     *mainThreadPool = rtThreadPool::globalInstance();

      DecodeImageData *decodeImageData = new DecodeImageData(this);
      rtThreadTask               *task = new rtThreadTask(decodeTextureData, decodeImageData, "");
      
      mainThreadPool->executeTask(task);
      mLoadTextureRequested = true;
    }

    return PX_OK;
  }

  virtual pxError unloadTextureData()
  {
    if (mInitialized)
    {
      if (mTexture)
      {
        int64_t bytes = getSurfaceByteSize(mTexture);
        context.adjustCurrentTextureMemorySize(-1 * bytes);

        mTexture->Release(mTexture);
        mTexture = NULL;
      }

      mOffscreenMutex.lock();
      mOffscreen.term();
      mFreeOffscreenDataRequested = false;
      mOffscreenMutex.unlock();

      mInitialized = false;
    }
    return PX_OK;
  }

  virtual pxError freeOffscreenData()
  {
    mOffscreenMutex.lock();
    if (mFreeOffscreenDataRequested)
    {
      rtLogDebug("freeing offscreen data");
      mOffscreen.term();
    }
    mFreeOffscreenDataRequested = false;
    mOffscreenMutex.unlock();
    return PX_OK;
  }

  virtual pxError bindGLTexture(int /*tLoc*/)
  {
   if (!mInitialized)
    {
      loadTextureData();
      return PX_NOTINITIALIZED;
    }

    if (!mTexture)
    {
      rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
      return PX_NOTINITIALIZED;
    }

    // TODO would be nice to do the upload in createTexture but right now it's getting called on wrong thread
    if (!mTextureUploaded)
    {
      if (!context.isTextureSpaceAvailable(this))
      {
        //attempt to free texture memory
        int64_t textureMemoryNeeded = context.textureMemoryOverflow(this);
        context.ejectTextureMemory(textureMemoryNeeded);
        
        if (!context.isTextureSpaceAvailable(this))
        {
          rtLogError("not enough texture memory remaining to create texture");
          unloadTextureData();
          return PX_FAIL;
        }
        else if (!mInitialized)
        {
          return PX_NOTINITIALIZED;
        }
      }

      createSurface(mOffscreen);

      boundTexture = mTexture;  TRACK_TEX_CALLS();
      mTextureUploaded = true;

      //free up unneeded offscreen memory
      freeOffscreenDataInBackground();
    }
    else
    {
      boundTexture = mTexture;  TRACK_TEX_CALLS();
    }

    return PX_OK;
  }

  virtual pxError bindGLTextureAsMask(int /*mLoc*/)
  {
    if (!mInitialized)
    {
      loadTextureData();
      return PX_NOTINITIALIZED;
    }

    if (!mTexture)
    {
      rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
      return PX_NOTINITIALIZED;
    }

    if (!mTextureUploaded)
    {
      if (!context.isTextureSpaceAvailable(this))
      {
        //attempt to free texture memory
        int64_t textureMemoryNeeded = context.textureMemoryOverflow(this);
        context.ejectTextureMemory(textureMemoryNeeded);
        if (!context.isTextureSpaceAvailable(this))
        {
          rtLogError("not enough texture memory remaining to create texture");
          unloadTextureData();
          return PX_FAIL;
        }
        else if (!mInitialized)
        {
          return PX_NOTINITIALIZED;
        }
      }

      createSurface(mOffscreen);

      boundTextureMask = mTexture;   TRACK_TEX_CALLS();
      mTextureUploaded = true;
      
      //free up unneeded offscreen memory
      freeOffscreenDataInBackground();
    }
    else
    {
        boundTextureMask = mTexture;   TRACK_TEX_CALLS();
    }

    return PX_OK;
  }

  virtual pxError getOffscreen(pxOffscreen& o)
  {
    if (!mInitialized)
    {
      return PX_NOTINITIALIZED;
    }

    if (mCompressedData != NULL)
    {
      pxLoadImage(mCompressedData, mCompressedDataSize, o);
    }

    return PX_OK;
  }

  virtual int width()  { return mWidth;  }
  virtual int height() { return mHeight; }

  pxError compressedDataWeakReference(char*& data, size_t& dataSize)
  {
    data = mCompressedData;
    dataSize = mCompressedDataSize;
    return PX_OK;
  }

private:

  void freeOffscreenDataInBackground()
  {
    mOffscreenMutex.lock();
    mFreeOffscreenDataRequested = true;
    mOffscreenMutex.unlock();

    rtLogDebug("request to free offscreen data");
    rtThreadPool *mainThreadPool = rtThreadPool::globalInstance();
    
    DecodeImageData *imageData = new DecodeImageData(this);
    rtThreadTask         *task = new rtThreadTask(cleanupOffscreen, imageData, "");
    
    mainThreadPool->executeTask(task);
  }

  void setCompressedData(const char* data, const size_t dataSize)
  {
    freeCompressedData();
    if (data == NULL)
    {
      mCompressedData = NULL;
      mCompressedDataSize = 0;
    }
    else
    {
      mCompressedData = new char[dataSize];
      mCompressedDataSize = dataSize;
      memcpy(mCompressedData, data, mCompressedDataSize);
      mTextureDataAvailable = true;
    }
  }

  pxError freeCompressedData()
  {
    if (mCompressedData != NULL)
    {
      delete [] mCompressedData;
      mCompressedData = NULL;
    }
    mCompressedDataSize = 0;
    mTextureDataAvailable = false;
    return PX_OK;
  }

  pxOffscreen mOffscreen;
  bool        mInitialized;

  int         mWidth;
  int         mHeight;

  bool        mTextureUploaded;
  bool        mTextureDataAvailable;
  bool        mLoadTextureRequested;
  bool        mFreeOffscreenDataRequested;

  rtMutex     mOffscreenMutex;
  char*       mCompressedData;
  size_t      mCompressedDataSize;

  IDirectFBSurface       *mTexture;
  DFBSurfaceDescription   dsc;

private:

  pxError createSurface(pxOffscreen& o)
  {
    if(mTexture)
    {
      //rtLogError("###    NOT EMPTY !!!   >>>     pxTextureOffscreen::createSurface()  ");
      
      int64_t bytes = getSurfaceByteSize(mTexture);
      context.adjustCurrentTextureMemorySize( -1 * bytes ); // RELEASE: pxTextureOffscreen (non-empty)
      
      mTexture->Release(mTexture);
    }
  
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    dsc.width                 = o.width();
    dsc.height                = o.height();
    dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT| DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT);
    dsc.pixelformat           = dfbPixelformat;
    dsc.preallocated[0].data  = o.base();      // Buffer is your data
    dsc.preallocated[0].pitch = o.width() * 4;
    dsc.preallocated[1].data  = NULL;          // Not using a back buffer
    dsc.preallocated[1].pitch = 0;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    IDirectFBSurface  *tmp;

    DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &tmp ) );

    int64_t bytes = getSurfaceByteSize(tmp);
    context.adjustCurrentTextureMemorySize( bytes ); // CREATE: pxTextureOffscreen (tmp)

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TODO: The creation of 'tmp' is merely to have a DFB surface as Source to Blit to a Destination surface in VIDEO memory.  IF used !
 
    dsc.width                 = o.width();
    dsc.height                = o.height();
    dsc.caps                  = PREFERRED_MEMORY;  // Use Video Memory  //   DSCAPS_PREMULTIPLIED
    dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
    dsc.pixelformat           = dfbPixelformat;

    DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &mTexture ) );

    /*int64_t*/ bytes = getSurfaceByteSize(mTexture);
    context.adjustCurrentTextureMemorySize( bytes ); // CREATE: pxTextureOffscreen

    mTexture->Clear(mTexture, 0, 0, 0, 0); // Transparent

#ifndef DEBUG_SKIP_BLIT
    DFB_CHECK(mTexture->Blit(mTexture, tmp, NULL, 0,0)); TRACK_DRAW_CALLS(); // blit copy to surface
#endif

//    DFB_CHECK(mTexture->SetRenderOptions(mTexture,
//            DFBSurfaceRenderOptions(DSRO_MATRIX /*| DSRO_ANTIALIAS*/) ));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    /*int64_t*/ bytes = getSurfaceByteSize(tmp);
    context.adjustCurrentTextureMemorySize( -1 * bytes ); // RELEASE: pxTextureOffscreen (tmp)
  
    tmp->Release(tmp);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    return PX_OK;
  }

}; // CLASS - pxTextureOffscreen


class pxSwTexture: public pxTextureOffscreen
{
public:
  pxSwTexture()  : mOffscreen(), /*mTextureName(0),*/ mRasterTextureCreated(false),  mInitialized(false)
  {
    //ctor
  };
  
  ~pxSwTexture()
  {
    deleteTexture();
    mOffscreen.term();
  };
  
  void init(int w, int h)
  {
    if(!mInitialized)
    {
      mWidth  = w;
      mHeight = h;
      
      mOffscreen.init(w,h);
      
      mOffscreen.setUpsideDown(true);
      
      mInitialized = true;
    }
  }

  void clear(const pxRect& r)
  {
    mOffscreen.fill(r, pxClear);
  }
  
  void clear()
  {
    mOffscreen.fill(pxClear);
  }
  
  pxError copy(int src_x, int src_y, int dst_x, int dst_y, float w, float h, pxOffscreen &o)
  {
#if 0
    // PSEUDO COLOR - SOURCE BUFFER ... fails
    float sw = 100, sh = 100;                           // HACK
    pxRect bFILL(src_x, src_y, src_x + sw, src_y + sh); // HACK
    o.fill(bFILL, pxBlue);                              // HACK
#endif
    
    o.blit(mOffscreen, dst_x, dst_y, w, h, src_x, src_y); // copy RASTER >> to >> RENDER offscreen
    
#if 0
    // PSEUDO COLOR - DESTINATION BUFFER ... works
    
    float dw = 100, dh = 100;                           // HACK
    pxRect rFILL(dst_x, dst_y, dst_x + dw, dst_y + dh); // HACK
    mOffscreen.fill(rFILL, pxGreen);                    // HACK
#endif

#if 0
#ifdef PX_PLATFORM_MAC
    // HACK
    // HACK
    // HACK
    static int frame = 20;
    if(frame-- == 0)
    {
      void *img_raster = makeNSImage(o.base(), o.width(), o.height(), 4);
      void *img_render = makeNSImage(mOffscreen.base(), mOffscreen.width(), mOffscreen.height(), 4);
      
      frame = -1;
    }
    // HACK
    // HACK
    // HACK
#endif
#endif

/*
    if (mTextureName != 0)
    {
      glBindTexture(GL_TEXTURE_2D, mTextureName);   TRACK_TEX_CALLS();
 
      // Upload (CRAWL) entire RENTER offscreen to TEXTURE on GPU
      glTexSubImage2D(GL_TEXTURE_2D, 0, dst_x, dst_y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, mOffscreen.base());
      
//      glBindTexture(GL_TEXTURE_2D, GL_NONE); // unbind
    }
    
    TODO:  DFB "blit" equivalent.  Maybe just drawImage() ?
*/
    return PX_OK;
  };

/*
  // baggage
  virtual inline int width()                        { return mWidth;  };
  virtual inline int height()                       { return mHeight; };
  virtual pxError getOffscreen(pxOffscreen& / *o* /)  { return PX_FAIL; };
  virtual pxError bindGLTextureAsMask(int / *mLoc* /) { return PX_FAIL; };

  virtual pxError deleteTexture()
  {
    if (mTextureName != 0)
    {
      glDeleteTextures(1, &mTextureName);
      mTextureName = 0;
      mRasterTextureCreated = false;
      context.adjustCurrentTextureMemorySize(-1 * mWidth * mHeight * 4); // FREE
    }
    mInitialized = false;
    return PX_OK;
  }
  
  virtual pxError bindGLTexture(int tLoc)
  {
    glActiveTexture(GL_TEXTURE1);
    
    if (!mRasterTextureCreated)
    {
      if (!context.isTextureSpaceAvailable(this))
      {
        //attempt to free texture memory
        int64_t textureMemoryNeeded = context.textureMemoryOverflow(this);
        context.ejectTextureMemory(textureMemoryNeeded);
        
        if (!context.isTextureSpaceAvailable(this))
        {
          rtLogError("not enough texture memory remaining to create raster texture");
          mInitialized = false;
          return PX_FAIL;
        }
        else if (!mInitialized)
        {
          return PX_NOTINITIALIZED;
        }
      }
      
      glGenTextures(1, &mTextureName);
      glBindTexture(GL_TEXTURE_2D, mTextureName);   TRACK_TEX_CALLS();
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, PX_TEXTURE_MIN_FILTER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, PX_TEXTURE_MAG_FILTER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, mOffscreen.base());
      
      context.adjustCurrentTextureMemorySize(mWidth * mHeight * 4); // USE
      mRasterTextureCreated = true;
      
      printf("\n SW TEXTURE >>  glGetError() = %d   >>  mWidth: %d   mHeight: %d\n", glGetError(), mWidth, mHeight);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, mTextureName);   TRACK_TEX_CALLS();
    }
    
    mInitialized = true;
    
    glUniform1i(tLoc, 1);
    return PX_OK;
  }
*/

private:
  int mWidth;
  int mHeight;
  
  pxOffscreen mOffscreen;
  //GLuint mTextureName;
  bool mRasterTextureCreated;
  bool mInitialized;
}; // CLASS - pxSwTexture

typedef rtRef<pxSwTexture> pxSwTextureRef;
static pxSwTextureRef  swRasterTexture; // aka "fullScreenTextureSoftware"
void onDecodeComplete(void* context, void* data)
{
  DecodeImageData* imageData = (DecodeImageData*)context;
  pxOffscreen* decodedOffscreen = (pxOffscreen*)data;
  
  if (imageData != NULL && decodedOffscreen != NULL)
  {
    // Populate 'textureOffscreen' with decoded image data.
    //
    pxTextureOffscreenRef texture = imageData->textureOffscreen;
    if (texture.getPtr() != NULL)
    {
      texture->createTexture(*decodedOffscreen);
    }
  }

  if (decodedOffscreen != NULL)
  {
    delete decodedOffscreen;
    decodedOffscreen = NULL;
    data = NULL;
  }

  if (imageData != NULL)
  {
    delete imageData;
    imageData = NULL;
  }
}

void decodeTextureData(void* data)
{
  if (data != NULL)
  {
    DecodeImageData* imageData = (DecodeImageData*)data;
    
    if(imageData == NULL)
    {
      rtLogError(" decodeTextureData() - imageData == NULL ... unexepcted");
      return;
    }
    
    char *compressedImageData = NULL;
    size_t compressedImageDataSize = 0;
    
    imageData->textureOffscreen->compressedDataWeakReference(compressedImageData, compressedImageDataSize);
    
    if (compressedImageData != NULL)
    {
      pxOffscreen *decodedOffscreen = new pxOffscreen();

      pxLoadImage(compressedImageData, compressedImageDataSize, *decodedOffscreen); // background image decode
      if (gUIThreadQueue)
      {
        gUIThreadQueue->addTask(onDecodeComplete, data, decodedOffscreen);
      }             // queue image onto UI thread. Task will 'delete' data.
    }
    else
    {
      if (gUIThreadQueue)
      {
        gUIThreadQueue->addTask(onDecodeComplete, data, NULL); // Just clean up !
      }
    }
  }
}

void onOffscreenCleanupComplete(void* context, void*)
{
  DecodeImageData* imageData = (DecodeImageData*)context;
  if (imageData != NULL)
  {
    delete imageData;
    imageData = NULL;
  }
}

void cleanupOffscreen(void* data)
{
  if (data != NULL)
  {
    DecodeImageData* imageData = (DecodeImageData*)data;

    if (imageData->textureOffscreen.getPtr() != NULL)
    {
      imageData->textureOffscreen->freeOffscreenData();
    }

    if (gUIThreadQueue)
    {
      gUIThreadQueue->addTask(onOffscreenCleanupComplete, data, NULL);
    }
  }
}

//====================================================================================================================================================================================

class pxTextureAlpha : public pxTexture
{
public:
  pxTextureAlpha() :   mDrawWidth(0.0),  mDrawHeight(0.0),
                      mImageWidth(0.0), mImageHeight(0.0),
                      mTexture(NULL),   mInitialized(false), mBuffer(NULL)
  {
    mTextureType = PX_TEXTURE_ALPHA;
  }

  pxTextureAlpha(float w, float h, float iw, float ih, void* buffer)
    :  mDrawWidth(w),   mDrawHeight(h),
      mImageWidth(iw), mImageHeight(ih),
      mTexture(NULL),  mInitialized(false), mBuffer(NULL)
  {
    mTextureType = PX_TEXTURE_ALPHA;

    // copy the pixels
    int bitmapSize = ih * iw;
    mBuffer = malloc(bitmapSize);

#if 1
    memcpy(mBuffer, buffer, bitmapSize);
#else
    // TODO consider iw,ih as ints rather than floats...
    int32_t bw = (int32_t)iw;
    int32_t bh = (int32_t)ih;

    // Flip here so that we match FBO layout...
    for (int32_t i = 0; i < bh; i++)
    {
      uint8_t *s = (uint8_t*)buffer+(bw*i);
      uint8_t *d = (uint8_t*)mBuffer+(bw*(bh-i-1));
      uint8_t *de = d+bw;
      while(d<de)
        *d++ = *s++;
    }
#endif

  }

  ~pxTextureAlpha()
  {
    if(mBuffer)
    {
      free(mBuffer);
    }
    deleteTexture();
  }

  void createTexture(float w, float h, float iw, float ih)
  {
    rtLogDebug("############# this: %p >>  %s  WxH: %.0f x %.0f \n", this, __PRETTY_FUNCTION__, w,h);

    if (mTexture != NULL)
    {
        deleteTexture();
    }

    if(iw == 0 || ih == 0)
    {
      return; // DIMENSIONLESS
    }

    mDrawWidth   = w;
    mDrawHeight  = h;
    mImageWidth  = iw;
    mImageHeight = ih;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Load DFB surface with TEXTURE

    dsc.width                 = iw;
    dsc.height                = ih;
    dsc.flags                 = DFBSurfaceDescriptionFlags( DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT | DSDESC_CAPS );
    dsc.caps                  = DSCAPS_SYSTEMONLY; // Slow memory
    dsc.pixelformat           = DSPF_A8;           // ALPHA !!
    dsc.preallocated[0].data  = mBuffer;           // Buffer is your data
    dsc.preallocated[0].pitch = iw;
    dsc.preallocated[1].data  = NULL;              // Not using a back buffer
    dsc.preallocated[1].pitch = 0;

    DFB_CHECK (dfb->CreateSurface( dfb, &dsc, &mTexture));

   // mTexture->Clear(mTexture, 0, 0, 0, 0); // Transparent

    DFB_CHECK(mTexture->SetRenderOptions(mTexture,
            DFBSurfaceRenderOptions(DSRO_MATRIX /*| DSRO_ANTIALIAS*/) ));

    int64_t bytes = getSurfaceByteSize(mTexture);
    context.adjustCurrentTextureMemorySize( bytes );

    mInitialized = true;
  }

  virtual pxError deleteTexture()
  {
    rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",mTexture, __PRETTY_FUNCTION__);

    if (mTexture != NULL)
    {
      int64_t bytes = getSurfaceByteSize(mTexture);
      context.adjustCurrentTextureMemorySize( -1 * bytes ); // RELEASE: pxTextureAlpha (ALPHA)

      mTexture->Release(mTexture);
      mTexture = NULL;
    }
    mInitialized = false;
    return PX_OK;
  }

  virtual pxError bindGLTexture(int /*tLoc*/)
  {
    // TODO Moved to here because of js threading issues
    if (!mInitialized) createTexture(mDrawWidth,mDrawHeight,mImageWidth,mImageHeight);
    if (!mInitialized)
    {
      //rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

      return PX_NOTINITIALIZED;
    }

    boundTexture = mTexture;

    return PX_OK;
  }

  virtual pxError bindGLTextureAsMask(int /*mLoc*/)
  {
    if (!mInitialized)
    {
      rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

      return PX_NOTINITIALIZED;
    }

    boundTextureMask = mTexture;

    return PX_OK;
  }

  virtual pxError getOffscreen(pxOffscreen& /*o*/)
  {
    rtLogDebug("############# this: %p >>  %s  ENTER\n", this, __PRETTY_FUNCTION__);

    if (!mInitialized)
    {
        return PX_NOTINITIALIZED;
    }
    return PX_FAIL;
  }

  virtual int width()  { return mDrawWidth;  }
  virtual int height() { return mDrawHeight; }

  //DFBVertex*              getVetricies()   { return &v[0];   };
  //DFBVertex v[4];  //quad

  IDirectFBSurface*       getSurface()     { return mTexture; };
  DFBSurfaceDescription   getDescription() { return dsc;     };

private:
  float mDrawWidth;
  float mDrawHeight;
  float mImageWidth;
  float mImageHeight;

  IDirectFBSurface       *mTexture;
  DFBSurfaceDescription   dsc;

  bool                    mInitialized;
  void                   *mBuffer;

}; // CLASS - pxTextureAlpha

//====================================================================================================================================================================================

// SOLID
inline pxError draw_SOLID(int resW, int resH, float* matrix, float alpha,
                       DFBRectangle &src, DFBRectangle &dst,
                       pxTextureRef texture, const float* color)
{
  (void) resW; (void) resH; (void) src; (void) matrix; (void) texture;

  // TRANSPARENT
  if(!color || color[3] == 0.0 || alpha == 0.0)
  {
    return PX_OK;
  }

  // Switch to COLORIZE for Glyphs...
  DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
        DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE | DSBLIT_BLEND_COLORALPHA | DSDRAW_SRC_PREMULTIPLY) ));

  DFB_CHECK(boundFramebuffer->SetColor( boundFramebuffer, color[0] * 255.0, // RGBA
                                                          color[1] * 255.0,
                                                          color[2] * 255.0,
                                               /*color[3] */ alpha * 255.0 ));
#ifndef DEBUG_SKIP_BLIT
  DFB_CHECK(boundFramebuffer->Blit(boundFramebuffer, boundTexture, NULL, dst.x , dst.y));
#else
  UNUSED_PARAM(dst);
#endif
  return PX_OK;
}

//====================================================================================================================================================================================

// TEXTURE
inline pxError draw_TEXTURE(int resW, int resH, float* matrix, float alpha,
                         DFBRectangle &src, DFBRectangle &dst,
                         pxTextureRef texture,
                         int32_t xStretch = 0, int32_t yStretch = 0)
{
  (void) resW; (void) resH; (void) matrix; (void) texture;

#ifndef DEBUG_SKIP_BLIT
  if(xStretch == pxConstantsStretch::REPEAT ||
     yStretch == pxConstantsStretch::REPEAT)
  {
    DFB_CHECK(boundFramebuffer->TileBlit(boundFramebuffer, boundTexture,  &src,  0,0));
  }
  else
  if(xStretch == pxConstantsStretch::STRETCH ||
     yStretch == pxConstantsStretch::STRETCH)
  {
    DFB_CHECK(boundFramebuffer->StretchBlit(boundFramebuffer, boundTexture, &src, &dst));
  }
  else
  {
    DFB_CHECK(boundFramebuffer->Blit(boundFramebuffer, boundTexture, NULL, 0,0));
  }
#else
  UNUSED_PARAM(src);
  UNUSED_PARAM(dst);
  UNUSED_PARAM(texture);
  UNUSED_PARAM(xStretch);
  UNUSED_PARAM(yStretch);
#endif // DEBUG_SKIP_BLIT
  return PX_OK;
}

//====================================================================================================================================================================================

// MASK
inline pxError draw_MASK(int resW, int resH, float* matrix, float alpha,
                      DFBRectangle /*&src*/, DFBRectangle /*&dst*/,
                      pxTextureRef texture, pxTextureRef mask,
                      pxConstantsMaskOperation::constants maskOp /*= pxConstantsMaskOperation::NORMAL*/)
{
  (void) resW; (void) resH; (void) matrix; (void) alpha; (void) texture;
  (void) maskOp; // prevent warning

  if (mask->bindGLTextureAsMask(0) != PX_OK)  // SETS >> 'boundTextureMask'
  {
    return PX_FAIL;
  }

  if(boundTextureMask == NULL)
  {
    rtLogError("ERROR: draw_MASK() - no 'mask'");
    return PX_OK;
  }

// TODO: ... Masking does not seem to work when blitting directly to the *primary* surface.
// TODO:     Use a temporary intermediate surface for Masking. - Performance will take a hit.

#if 1

    int w = 0, h = 0;
    boundFramebuffer->GetSize(boundTextureMask, &w, &h);

    IDirectFBSurface*   tmpSurf = NULL;

    // -------------------------------------------------------------------------------------------------------
    //
    // HACK: Create temporary intermediate surface...
    //
    //       boundFramebuffer == dfbSurface == 'primary' surface which does not seem to support direct masking.
    //
    DFBSurfaceDescription desc;

    memset(&desc, 0, sizeof(desc));
    desc.flags       = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT);
    desc.pixelformat = dfbPixelformat;
    desc.width       = w;
    desc.height      = h;

    DFB_CHECK( dfb->CreateSurface( dfb, &desc, &tmpSurf ) );

    // CLEAR EXTRA
    tmpSurf->Clear(tmpSurf, 0, 0, 0, 0); // CLEAR (transparent)

    // -------------------------------------------------------------------------------------------------------
    //
    // MASKING to offscreen surface
    //
    if(tmpSurf && boundTexture && boundTextureMask)
    {
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // ReportSurfaceInfo("tmpSurf", tmpSurf);        // JUNK
      // ReportSurfaceInfo("dfbSurface", dfbSurface);  // JUNK
      // ReportSurfaceInfo("outsideDfbSurface", outsideDfbSurface);  // JUNK
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      tmpSurf->SetSourceMask(tmpSurf, boundTextureMask, 0, 0, DSMF_NONE);                 // Use 'maskSurf' as MASK
      tmpSurf->SetBlittingFlags(tmpSurf,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_SRC_MASK_ALPHA ) ); // Set BLEND + MASK flags !

      tmpSurf->Blit(tmpSurf,   boundTexture, NULL, 0, 0);            // Mask 'imageSurf' -> 'tmpSurf' ... using 'maskSurf'
      tmpSurf->SetSourceMask(tmpSurf, NULL, 0, 0, DSMF_NONE);        // Clear the mask
      tmpSurf->SetBlittingFlags(tmpSurf, DSBLIT_BLEND_ALPHACHANNEL); // Restore the blitting flags

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Output....

     // TODO:  maskOp will affect the blending functions here.

      DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
            DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) ));

      boundFramebuffer->Blit(boundFramebuffer, tmpSurf, NULL, 0, 0);   // Final Blit
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      tmpSurf->Release(tmpSurf);
      tmpSurf = NULL;
    }//ENDIF
    // -------------------------------------------------------------------------------------------------------

#else

  if(boundFramebuffer && boundTextureMask && boundTexture)
  {
    boundFramebuffer->SetSourceMask(boundFramebuffer, boundTextureMask, 0, 0, DSMF_NONE); // Set the mask
    boundFramebuffer->SetBlittingFlags(boundFramebuffer,
            DFBSurfaceBlittingFlags(DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_SRC_MASK_ALPHA ) );

#ifndef DEBUG_SKIP_BLIT
    DFB_CHECK(boundFramebuffer->Blit(boundFramebuffer, boundTexture, NULL, 0, 0));
#endif

    boundFramebuffer->SetSourceMask(boundFramebuffer, NULL, 0, 0, DSMF_NONE); // Clear the mask

#ifndef DEBUG_SKIP_ALPHA_BLEND
      DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
            DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) )); // Restore
#endif
  }

#endif // 0
  return PX_OK;
}

//====================================================================================================================================================================================

static void drawRect2(float x, float y, float w, float h, const float* c)
{
  // args are tested at call site...

  if( boundFramebuffer == NULL)
  {
    rtLogError("cannot drawRect2() on context surface because surface is NULL");
    return;
  }

  float colorPM[4];
  premultiply(colorPM, c);

  // 'applyMatrix()' performed by caller

  // Opacity ?
  if( (/*colorPM[3] */ gAlpha) < 0.99)
  {
    DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
         DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL |  DSBLIT_BLEND_COLORALPHA | DSBLIT_COLORIZE ) ));

    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_BLEND); // use alpha from color.
  }
  else
  {
    DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
         DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE ) ));

    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX);
  }

  DFB_CHECK( boundFramebuffer->SetColor( boundFramebuffer, colorPM[0] * 255.0, // RGBA
                                                           colorPM[1] * 255.0,
                                                           colorPM[2] * 255.0,
                                                           colorPM[3] * 255.0 * gAlpha));

  DFB_CHECK( boundFramebuffer->FillRectangle( boundFramebuffer, x, y, w, h));

  // Turn off Alpha
 if( (/*colorPM[3] */ gAlpha) < 0.99)
  {
    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX);
  }

  // Restore ?
  DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) ));
}


static void drawRectOutline(float x, float y, float w, float h, float lw, const float* c)
{
  // args are tested at call site...

  if( boundFramebuffer == NULL)
  {
    rtLogError("cannot drawRectOutline() on context surface because surface is NULL");
    return;
  }

  float colorPM[4];
  premultiply(colorPM, c);

  // 'applyMatrix()' performed by caller

  // Opacity ?
//  if( (/* colorPM[3] */ gAlpha) < 0.99)
  {
    DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
         DFBSurfaceBlittingFlags( DSBLIT_BLEND_COLORALPHA | DSBLIT_COLORIZE ) ));
  }

  DFB_CHECK( boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_BLEND) );
  DFB_CHECK( boundFramebuffer->SetColor( boundFramebuffer, colorPM[0] * 255.0, // RGBA
                                                           colorPM[1] * 255.0,
                                                           colorPM[2] * 255.0,
                                                           colorPM[3] * 255.0 * gAlpha));
  if(lw >= w)
  {
    lw = 0;
  }

  // Avoid 'narrowing conversion' warning...
  int xx  = (int) x;
  int yy  = (int) y;
  int ww  = (int) w;
  int hh  = (int) h;
  int llw = (int) lw;

  DFBRectangle rects[4];

  rects[0].x = xx + llw;      rects[0].y = yy;            rects[0].w = ww - 2 * llw;    rects[0].h = llw; // TOP
  rects[1].x = xx + llw;      rects[1].y = yy+ hh - llw;  rects[1].w = ww - 2 * llw;    rects[1].h = llw; // Bottom
  rects[2].x = xx;            rects[2].y = yy;            rects[2].w = llw;             rects[2].h = hh;  // Left
  rects[3].x = xx + ww - llw; rects[3].y = yy;            rects[3].w = llw;             rects[3].h = hh;  // Right

/*
  C++11...

  DFBRectangle rects[] =  {
                            { x,          y,            w, lw },
                            { x,          y + h - lw,   w, lw },
                            { x,          y,           lw, h  },
                            { x + w - lw, y,           lw, h  },
                          };
*/

  DFB_CHECK( boundFramebuffer->FillRectangles( boundFramebuffer, rects, 4 ) ); // border

  // Turn off Alpha
 if( (/*colorPM[3] */ gAlpha) < 0.99)
  {
    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX);
  }

  // Restore ?
  DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL ) ));

#ifndef DEBUG_SKIP_FLIPPING
  needsFlip = true;
#endif
}

static void drawImageTexture(float x, float y, float w, float h, pxTextureRef texture,
                             pxTextureRef mask, bool useTextureDimsAlways, float* color, // default: "color = BLACK"
                             pxConstantsStretch::constants xStretch,
                             pxConstantsStretch::constants yStretch,
                             pxConstantsMaskOperation::constants maskOp /*= pxConstantsMaskOperation::constants::NORMAL*/)
{
  // args are tested at call site...
  (void) maskOp; // prevent warning

  if (boundFramebuffer == NULL)
  {
    return; // Nowhere to Draw
  }

  // 'texture' not NULL... tested by caller
  float iw = texture->width();
  float ih = texture->height();

  if(useTextureDimsAlways)
  {
      w = iw;
      h = ih;
  }
  else
  {
    if (w == -1)
      w = iw;
    if (h == -1)
      h = ih;
  }

   if (iw <= 0 || ih <= 0)
   {
     rtLogError("ERROR: drawImageTexture() >>>  WxH: 0x0   BAD !");
     return; // DIMENSIONLESS
   }

  if (xStretch == pxConstantsStretch::NONE)  w = iw;
  if (yStretch == pxConstantsStretch::NONE)  h = ih;

#if 0 // redundant for DFB
  const float verts[4][2] =
  {
    { x,     y },
    { x+w,   y },
    { x,   y+h },
    { x+w, y+h }
  };

  float tw;
  switch(xStretch) {
  case pxConstantsStretch::NONE:
    tw = w/iw;
    break;
  case pxConstantsStretch::STRETCH:
    tw = 1.0;
    break;
  case pxConstantsStretch::REPEAT:
    tw = w/iw;
    break;
  }

  float th;
  switch(yStretch) {
  case pxConstantsStretch::NONE:
    th = h/ih;
    break;
  case pxConstantsStretch::STRETCH:
    th = 1.0;
    break;
  case pxConstantsStretch::REPEAT:
#if 1 // PX_TEXTURE_ANCHOR_BOTTOM
    th = h/ih;
#else

    float temp = h/ih;
    th = ceil(temp);
    tb = 1.0f-(temp-floor(temp));
#endif
    break;
  }
#endif // 00

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  DFBRectangle src; // uses INT for (x,y, w, h)

  src.x = 0;  src.y = 0;
  src.w = iw; src.h = ih;

  DFBRectangle dst; // uses INT for (x,y, w, h)

  dst.x = x; dst.y = y;
  dst.w = w; dst.h = h;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if 0
  // TEXTURED TRIANGLES...

  if(boundTexture != NULL)
  {
    pxFBOTexture *tex = (pxFBOTexture *) texture.getPtr();

    rtLogDebug("\nINFO: ###################  TextureTriangles()  !!!!!!!!!!!!!!!!");
    dfbSurface->TextureTriangles(dfbSurface, tex->getSurface(), tex->getVetricies(), NULL, 4, DTTF_STRIP);
  }
  else
  {
    rtLogDebug("\nINFO: ###################  TextureTriangles() fails");
  }
#endif

  bool textureBindFailure = false;
  if (texture->bindGLTexture(0) != PX_OK)    // SETS >> 'boundTexture'
  {
    textureBindFailure = true;
  }

  applyMatrix(boundFramebuffer, gMatrix.data());

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Enable OPACITY ??
  if( (gAlpha * color[3]) < 0.99)
  {
    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_BLEND);
    boundFramebuffer->SetColor( boundFramebuffer, 255,255,255, gAlpha * 255 ); // RGBA

    boundFramebuffer->SetBlittingFlags(boundFramebuffer,
         DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL |  DSBLIT_BLEND_COLORALPHA) );
  }
  else
  {
    boundFramebuffer->SetBlittingFlags(boundFramebuffer,
        DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) );
  }

#ifndef DEBUG_SKIP_ALPHA_BLEND_FUNC
  boundFramebuffer->SetSrcBlendFunction(boundFramebuffer, DSBF_SRCALPHA);
  boundFramebuffer->SetDstBlendFunction(boundFramebuffer, DSBF_INVSRCALPHA);
#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  static float blackColor[4] = {0.0, 0.0, 0.0, 1.0};

  if (mask.getPtr() != NULL)
  {
      if (textureBindFailure || draw_MASK(gResW, gResH, gMatrix.data(), gAlpha, src, dst, texture, mask, maskOp) != PX_OK)
      {
        drawRect2(0, 0, iw, ih, blackColor);
      }
  }
  else
  if (texture->getType() != PX_TEXTURE_ALPHA)
  {
    if (textureBindFailure || draw_TEXTURE(gResW, gResH, gMatrix.data(), gAlpha, src, dst, texture, xStretch, yStretch) != PX_OK)
    {
      drawRect2(0, 0, iw, ih, blackColor);
    }
  }
  else // PX_TEXTURE_ALPHA
  {
    float colorPM[4];
    premultiply(colorPM, color);

    if (draw_SOLID(gResW, gResH, gMatrix.data(), gAlpha, src, dst, texture, colorPM) != PX_OK)
    {
      drawRect2(0, 0, iw, ih, blackColor);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Disable OPACITY ??
  if( (gAlpha * color[3]) < 0.99)
  {
    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX);

#ifndef DEBUG_SKIP_ALPHA_BLEND_FUNC
    boundFramebuffer->SetSrcBlendFunction(boundFramebuffer, DSBF_ONE);   // restore ?
    boundFramebuffer->SetDstBlendFunction(boundFramebuffer, DSBF_ZERO);
#endif
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef DEBUG_SKIP_ALPHA_BLEND
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Restore ?
  DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) ));
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#endif

#ifndef DEBUG_SKIP_FLIPPING
  needsFlip = true;
#endif
}

static void drawImage92(float x,  float y,  float w,  float h,
                        float x1, float y1, float x2, float y2, pxTextureRef texture)
{
  // args are tested at call site...

  if (boundFramebuffer == NULL)
  {
    rtLogError("cannot 'drawImage92()' on context surface because boundFramebuffer is NULL");
    return; // Nowhere to Draw
  }

  DFBRectangle srcUL, dstUL; // Upper Left;
  DFBRectangle srcUM, dstUM; // Upper Middle;
  DFBRectangle srcUR, dstUR; // Upper Right

  DFBRectangle srcML, dstML; // Middle Left;
  DFBRectangle srcMM, dstMM; // Middle Middle;
  DFBRectangle srcMR, dstMR; // Middle Right

  DFBRectangle srcBL, dstBL; // Bottom Left;
  DFBRectangle srcBM, dstBM; // Bottom Middle;
  DFBRectangle srcBR, dstBR; // Bottom Right

  float iw = texture->width();
  float ih = texture->height();

  // HACK ! .. prevent 'zero' dimension rectangles getting to blitter.
  //
  if(x1 <=0) x1 = 1;
  if(y1 <=0) y1 = 1;
  if(x2 <=0) x2 = 1;
  if(y2 <=0) y2 = 1;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // TOP ROW
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  srcUL.w = x1;                    srcUM.w = iw - (x1 + x2);        srcUR.w = x2;
  srcUL.h = y1;                    srcUM.h = srcUL.h;               srcUR.h = srcUL.h;
  srcUL.x = 0;                     srcUM.x = x1;                    srcUR.x = srcUM.x + srcUM.w;
  srcUL.y = 0;                     srcUM.y = srcUL.y;               srcUR.y = srcUL.y;

  dstUL.w = x1;                    dstUM.w = w - (x1 + x2);         dstUR.w = x2;
  dstUL.h = y1;                    dstUM.h = dstUL.h;               dstUR.h = dstUL.h;
  dstUL.x = x;                     dstUM.x = dstUL.x + dstUL.w;     dstUR.x = dstUM.x + dstUM.w;
  dstUL.y = y;                     dstUM.y = dstUL.y;               dstUR.y = dstUL.y;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // MIDDLE ROW
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  srcML.w = srcUL.w;               srcMM.w = srcUM.w;               srcMR.w = srcUR.w;
  srcML.h = ih - (y1 + y2);        srcMM.h = srcML.h;               srcMR.h = srcML.h;
  srcML.x = 0;                     srcMM.x = x1;                    srcMR.x = srcMM.x + srcMM.w;
  srcML.y = y1;                    srcMM.y = srcML.y;               srcMR.y = srcML.y;

  dstML.w = dstUL.w;               dstMM.w = dstUM.w;               dstMR.w = dstUR.w;
  dstML.h = h - (y1 + y2);         dstMM.h = dstML.h;               dstMR.h = dstML.h;
  dstML.x = dstUL.x;               dstMM.x = dstUM.x;               dstMR.x = dstUR.x;
  dstML.y = dstUL.y + dstUL.h;     dstMM.y = dstUM.y + dstUM.h;     dstMR.y = dstUR.y + dstUR.h;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BOTTOM ROW
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  srcBL.w = srcML.w;               srcBM.w = srcMM.w;               srcBR.w = srcMR.w;
  srcBL.h = y2;                    srcBM.h = srcBL.h;               srcBR.h = srcBM.h;
  srcBL.x = 0;                     srcBM.x = x1;                    srcBR.x = srcBM.x + srcBM.w;
  srcBL.y = (ih - y2);             srcBM.y = srcBL.y;               srcBR.y = srcBL.y;

  dstBL.w = dstML.w;               dstBM.w = dstMM.w;               dstBR.w = dstMR.w;
  dstBL.h = y2;                    dstBM.h = dstBL.h;               dstBR.h = dstBL.h;
  dstBL.x = dstUL.x;               dstBM.x = dstMM.x;               dstBR.x = dstMR.x;
  dstBL.y = dstML.y + dstML.h;     dstBM.y = dstMM.y + dstMM.h;     dstBR.y = dstMR.y + dstMR.h;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool textureBindFailure = false;
  if (texture->bindGLTexture(0) != PX_OK)
  {
    textureBindFailure = true;
  }

  boundFramebuffer->SetRenderOptions( boundFramebuffer, DSRO_MATRIX );//| DSRO_ANTIALIAS );

  //                                   UPPER ROW              MIDDLE ROW            BOTTOM ROW
  const DFBRectangle src[] = { srcUL, srcUM, srcUR,   srcML, srcMM, srcMR,    srcBL, srcBM, srcBR };
  const DFBRectangle dst[] = { dstUL, dstUM, dstUR,   dstML, dstMM, dstMR,    dstBL, dstBM, dstBR };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Enable OPACITY ??

  if(gAlpha < 0.99)
  {
#ifndef DEBUG_SKIP_ALPHA_BLEND_FUNC
   boundFramebuffer->SetSrcBlendFunction(boundFramebuffer, DSBF_SRCALPHA);
   boundFramebuffer->SetDstBlendFunction(boundFramebuffer, DSBF_INVSRCALPHA);
#endif

    DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
         DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA ) ));

    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_BLEND); // use alpha from color.
    boundFramebuffer->SetColor( boundFramebuffer, 255,255,255, 255 * gAlpha ); // RGBA
  }
  else
  {
    DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL ) ));

    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  applyMatrix(boundFramebuffer, gMatrix.data());

#ifndef DEBUG_SKIP_BLIT

  // BLIT the 9 SLICE
  if (textureBindFailure)
  {
    static float blackColor[4] = {0.0, 0.0, 0.0, 1.0};
    drawRect2(0, 0, iw, ih, blackColor);
  }
  else
  {
    DFB_CHECK(boundFramebuffer->BatchStretchBlit(boundFramebuffer, boundTexture, src, dst,
                                                 sizeof(src) / sizeof(DFBRectangle)));
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Disable OPACITY ??
  if(gAlpha < 0.99)
  {
#ifndef DEBUG_SKIP_ALPHA_BLEND_FUNC
    boundFramebuffer->SetSrcBlendFunction(boundFramebuffer, DSBF_SRCALPHA);   // restore ?
    boundFramebuffer->SetDstBlendFunction(boundFramebuffer, DSBF_DESTALPHA);
#endif

    boundFramebuffer->SetDrawingFlags(boundFramebuffer, DSDRAW_NOFX); //disable
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Restore ?
  DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer,
          DFBSurfaceBlittingFlags( DSBLIT_BLEND_ALPHACHANNEL) ));

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#endif

}

bool gContextInit = false;

pxContext::~pxContext()
{
  //TODO

  boundFramebuffer = NULL;
  boundTextureMask = NULL;
  boundTexture     = NULL;
}

void pxContext::init()
{
#if 0
  if (gContextInit)
    return;
  else
    gContextInit = true;
#endif

#ifdef ENABLE_DFB_GENERIC
  dfb = outsideDfb;
  dfbSurface = outsideDfbSurface;
#endif //ENABLE_DFB_GENERIC

  DFB_CHECK( dfbSurface->GetPixelFormat(dfbSurface, &dfbPixelformat) );

  DFB_CHECK( dfbSurface->Clear( dfbSurface, 0x00, 0x00, 0x00, 0x00 ) ); // TRANSPARENT

  boundFramebuffer = dfbSurface;  // needed here.
  boundTexture     = dfbSurface;  // needed here.

  DFB_CHECK(dfbSurface->SetRenderOptions(dfbSurface, DSRO_MATRIX));

#ifndef DEBUG_SKIP_CLEAR
  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer, 0x00, 0x00, 0x00, 0x00 ) ); // TRANSPARENT
#else
  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer, 0x00, 0xFF, 0x00, 0xFF ) );  //  CLEAR_GREEN   << JUNK
#endif

  DFB_CHECK(boundFramebuffer->SetRenderOptions(boundFramebuffer,
          DFBSurfaceRenderOptions(DSRO_MATRIX /*| DSRO_ANTIALIAS*/) ));

  rtValue val;
  if (RT_OK == rtSettings::instance()->value("enableTextureMemoryMonitoring", val))
  {
    mEnableTextureMemoryMonitoring = val.toString().compare("true") == 0;
  }
  if (RT_OK == rtSettings::instance()->value("textureMemoryLimitInMb", val))
  {
    setTextureMemoryLimit((int64_t)val.toInt32() * (int64_t)1024 * (int64_t)1024);
  }

  rtLogSetLevel(RT_LOG_INFO); // LOG LEVEL

  std::srand(unsigned (std::time(0)));
}


void pxContext::term()  // clean up statics 
{
  swRasterTexture = NULL;
}

void pxContext::setSize(int w, int h)
{
  gResW = w;
  gResH = h;

  if (currentFramebuffer == defaultFramebuffer)
  {
    defaultContextSurface.width  = w;
    defaultContextSurface.height = h;
    gResW = w;
    gResH = h;
  }
}

void pxContext::getSize(int& w, int& h)
{
   w = gResW;
   h = gResH;
}

void pxContext::clear(int /*w*/, int /*h*/)
{
  if(boundFramebuffer == NULL)
  {
    rtLogError("cannot 'clear(w,h)' on context surface because boundFramebuffer is NULL");
    return;
  }

#ifndef DEBUG_SKIP_CLEAR
  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer, 0x00, 0x00, 0x00, 0x00 ) ); // TRANSPARENT
#else
  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer, 0x00, 0x00, 0x8F, 0xFF ) );  //  CLEAR_BLUE   << JUNK
#endif
}

void pxContext::clear(int /*w*/, int /*h*/, float* fillColor )
{
  if(boundFramebuffer == NULL)
  {
    rtLogError("cannot 'clear(w,h, fill)' on context surface because boundFramebuffer is NULL");
    return;
  }

#ifndef DEBUG_SKIP_CLEAR
  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer,  fillColor[0] * 255.0, // RGBA
                                                         fillColor[1] * 255.0,
                                                         fillColor[2] * 255.0,
                                                         fillColor[3] * 255.0));
#else
  UNUSED_PARAM(fillColor);

  DFB_CHECK( boundFramebuffer->Clear( boundFramebuffer, 0x00, 0x00, 0x8F, 0xFF ) );  //  CLEAR_BLUE   << JUNK
#endif
  currentFramebuffer->enableDirtyRectangles(false);
}

void pxContext::clear(int left, int top, int right, int bottom)
{
#ifdef PX_DIRTY_RECTANGLES
  currentFramebuffer->setDirtyRectangle(left, top, right+left, bottom+top);
  currentFramebuffer->enableDirtyRectangles(true);
#endif

  right = right+left;
  bottom = bottom+top;
  left = left<0 ? 0 : left;
  top = top < 0 ? 0 : top;
  right = right > gResW ? gResW : right;
  bottom = bottom > gResH ? gResH : bottom;
  
  DFBRegion clip= { left, top, right, bottom };
  boundFramebuffer->SetClip( boundFramebuffer, &clip );
}

void pxContext::enableClipping(bool)
{
  //not needed for DFB
}


// JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK
// JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK

typedef uint32_t Fixed;

#define FRACT_BITS 16
#define FRACT_BITS_D2 8
#define FIXED_ZERO       (0)
#define FIXED_ONE        (1   << FRACT_BITS)
#define INT2FIXED(x)     ((x) << FRACT_BITS)
#define FLOAT2FIXED(x)   ((Fixed)((x) * (1 << FRACT_BITS)))
#define FIXED2INT(x)     ((x) >> FRACT_BITS)
#define FIXED2FLOAT(x)   ( ((float)  (x)) / (1 << FRACT_BITS) )
#define FIXED2DOUBLE(x)  ( ((double) (x)) / (1 << FRACT_BITS) )
#define MULT(x, y)       ( ((x) >> FRACT_BITS_D2) * ((y)>> FRACT_BITS_D2) )

#define SHIFT_AMOUNT FRACT_BITS //16 // 2^16 = 65536
#define SHIFT_MASK ((1 << SHIFT_AMOUNT) - 1) // 65535 (all LSB set, all MSB clear)


/*
 OUTPUT :-

  float_val = 0.160000     fixed_val = 0.159988  (0.000012)       sorta_val =  0.159988  (0.000012)
  float_val = 0.320000     fixed_val = 0.319992  (0.000008)       sorta_val =  0.319992  (0.000008)
  float_val = 0.480000     fixed_val = 0.479996  (0.000004)       sorta_val =  0.479996  (0.000004)
  float_val = 0.640000     fixed_val = 0.639999  (0.000001)       sorta_val =  0.639999  (0.000001)
  float_val = 0.800000     fixed_val = 0.799988  (0.000012)       sorta_val =  0.799988  (0.000012)
  float_val = 0.960000     fixed_val = 0.959991  (0.000009)       sorta_val =  0.959991  (0.000009)
  float_val = 1.120000     fixed_val = 1.119995  (0.000005)       sorta_val =  1.119995  (0.000005)
  float_val = 1.280000     fixed_val = 1.279999  (0.000001)       sorta_val =  1.279999  (0.000001)
  float_val = 1.440000     fixed_val = 1.439987  (0.000013)       sorta_val =  1.439987  (0.000013)
  float_val = 1.600000     fixed_val = 1.599991  (0.000009)       sorta_val =  1.599991  (0.000009)
  float_val = 1.760000     fixed_val = 1.759995  (0.000005)       sorta_val =  1.759995  (0.000005)
  float_val = 1.920000     fixed_val = 1.919998  (0.000002)       sorta_val =  1.919998  (0.000002)
  float_val = 2.080000     fixed_val = 2.079987  (0.000013)       sorta_val =  2.079987  (0.000013)
  float_val = 2.240000     fixed_val = 2.239990  (0.000010)       sorta_val =  2.239990  (0.000010)
*/

// JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK
// JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK JUNK


inline void applyMatrix(IDirectFBSurface  *surface, const float *mm)
{
#ifdef DEBUG_SKIP_APPLY
#warning " 'DEBUG_SKIP_APPLY' is Enabled"
  return;
#endif

  if(surface == NULL || mm == NULL)
  {
    rtLogError("cannot applyMatrix()  ... NULL params");
    return;
  }

  s32 matrix[9];

#if 1
  // Convert to fixed point for DFB...
  //
  matrix[0] = (s32)(mm[0]  * F2F_SHIFT);
  matrix[1] = (s32)(mm[4]  * F2F_SHIFT);
  matrix[2] = (s32)(mm[12] * F2F_SHIFT);
  matrix[3] = (s32)(mm[1]  * F2F_SHIFT);
  matrix[4] = (s32)(mm[5]  * F2F_SHIFT);
  matrix[5] = (s32)(mm[13] * F2F_SHIFT);

  matrix[6] = 0x00000;
  matrix[7] = 0x00000;
  matrix[8] = F2F_SHIFT;

//Fixed  matrix[9];

  // matrix[0] = FIXED(mm[0] );
  // matrix[1] = FIXED(mm[4] );
  // matrix[2] = FIXED(mm[12]);
  // matrix[3] = FIXED(mm[1] );
  // matrix[4] = FIXED(mm[5] );
  // matrix[5] = FIXED(mm[13]);

  // matrix[6] = FIXED(0);
  // matrix[7] = FIXED(0);
  // matrix[8] = FIXED(1);
#else

  // Identity Matrix ... for debugging.
  //
  matrix[0] = F2F_SHIFT;
  matrix[1] = 0x00000;
  matrix[2] = 0x00000;

  matrix[3] = 0x00000;
  matrix[4] = F2F_SHIFT;
  matrix[5] = 0x00000;

  matrix[6] = 0x00000;
  matrix[7] = 0x00000;
  matrix[8] = F2F_SHIFT;
#endif

  DFB_CHECK(surface->SetMatrix(surface, matrix));
}

void pxContext::setMatrix(pxMatrix4f& m)
{
#ifdef DEBUG_SKIP_MATRIX
#warning " 'DEBUG_SKIP_MATRIX' is Enabled"
  return;
#endif

  gMatrix.multiply(m);

  applyMatrix(dfbSurface, gMatrix.data());
}

pxMatrix4f pxContext::getMatrix()
{
  return gMatrix;
}

void pxContext::setAlpha(float a)
{
  gAlpha *= a;
}

float pxContext::getAlpha()
{
  return gAlpha;
}

pxContextFramebufferRef pxContext::createFramebuffer(int width, int height, bool /*antiAliasing*/)
{
  pxContextFramebuffer *fbo     = new pxContextFramebuffer();
  pxFBOTexture         *texture = new pxFBOTexture();

  texture->createTexture(width, height);

  fbo->setTexture(texture);

  return fbo;
}

pxError pxContext::updateFramebuffer(pxContextFramebufferRef fbo, int width, int height)
{
  if (fbo.getPtr() == NULL || fbo->getTexture().getPtr() == NULL)
  {
    return PX_FAIL;
  }

  return fbo->getTexture()->resizeTexture(width, height);
}

pxContextFramebufferRef pxContext::getCurrentFramebuffer()
{
  return currentFramebuffer;
}

pxError pxContext::setFramebuffer(pxContextFramebufferRef fbo)
{
  if (fbo.getPtr() == NULL || fbo->getTexture().getPtr() == NULL)
  {
    //glViewport ( 0, 0, defaultContextSurface.width, defaultContextSurface.height);

    gResW = defaultContextSurface.width;
    gResH = defaultContextSurface.height;

    // TODO probably need to save off the original FBO handle rather than assuming zero
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);  TRACK_FBO_CALLS();

    boundFramebuffer   = dfbSurface; //default
    currentFramebuffer = defaultFramebuffer;

    pxContextState contextState;
    currentFramebuffer->currentState(contextState);

    gAlpha  = contextState.alpha;
    gMatrix = contextState.matrix;

#ifdef PX_DIRTY_RECTANGLES
    if (currentFramebuffer->isDirtyRectanglesEnabled())
    {
      pxRect dirtyRect = currentFramebuffer->dirtyRectangle();
      DFBRegion clip= { dirtyRect.left(), dirtyRect.top(), dirtyRect.right(), dirtyRect.bottom() };
      boundFramebuffer->SetClip( boundFramebuffer, &clip );
    }
    else
    {
      DFBRegion clip= { 0, 0, gResW, gResH };
      boundFramebuffer->SetClip( boundFramebuffer, &clip );
    }
#endif //PX_DIRTY_RECTANGLES
    return PX_OK;
  }

  currentFramebuffer = fbo;
  pxContextState contextState;
  currentFramebuffer->currentState(contextState);
  gAlpha  = contextState.alpha;
  gMatrix = contextState.matrix;

#ifdef PX_DIRTY_RECTANGLES
  if (currentFramebuffer->isDirtyRectanglesEnabled())
  {
    pxRect dirtyRect = currentFramebuffer->dirtyRectangle();
    DFBRegion clip= { dirtyRect.left(), dirtyRect.top(), dirtyRect.right(), dirtyRect.bottom() };
    boundFramebuffer->SetClip( boundFramebuffer, &clip );
  }
  else
  {
     DFBRegion clip= { 0, 0, gResW, gResH };
     boundFramebuffer->SetClip( boundFramebuffer, &clip );
  }
#endif //PX_DIRTY_RECTANGLES

  return fbo->getTexture()->prepareForRendering();
}

#if 0
pxError pxContext::deleteContextSurface(pxTextureRef texture)
{
  if (texture.getPtr() == NULL)
  {
    return PX_FAIL;
  }
  return texture->deleteTexture();
}
#endif

void pxContext::enableDirtyRectangles(bool enable)
{
  currentFramebuffer->enableDirtyRectangles(enable);
  if (enable)
  {
    pxRect dirtyRect = currentFramebuffer->dirtyRectangle();
    DFBRegion clip= { dirtyRect.left(), dirtyRect.top(), dirtyRect.right(), dirtyRect.bottom() };
    boundFramebuffer->SetClip( boundFramebuffer, &clip );
  }
  else
  {
    DFBRegion clip= { 0, 0, gResW, gResH };
    boundFramebuffer->SetClip( boundFramebuffer, &clip );
  }
}

void pxContext::drawRect(float w, float h, float lineWidth, float* fillColor, float* lineColor)
{
#ifdef DEBUG_SKIP_RECT
#warning "DEBUG_SKIP_RECT enabled ... Skipping "
  return;
#endif

  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    return;
  }

  // COLORLESS
  if(fillColor == NULL && lineColor == NULL)
  {
    //rtLogError("cannot drawRect() on context surface because colors are NULL");
    return;
  }

  if(boundTexture == NULL)
  {
    rtLogError("cannot drawRect() on context surface because boundTexture is NULL");
    return;
  }

  applyMatrix(boundFramebuffer,gMatrix.data());

  // Fill ...
  if(fillColor != NULL && fillColor[3] > 0.0) // with non-transparent color
  {
    float half = lineWidth/2;
    drawRect2(half, half, w-lineWidth, h-lineWidth, fillColor);
  }

  // Frame ...
  if(lineColor != NULL && lineColor[3] > 0.0 && lineWidth > 0) // with non-transparent color and non-zero stroke
  {
    drawRectOutline(0, 0, w, h, lineWidth, lineColor);
  }
}

void pxContext::drawImage9(float w, float h, float x1, float y1,
                           float x2, float y2, pxTextureRef texture)
{
#ifdef DEBUG_SKIP_IMAGE9
#warning "DEBUG_SKIP_IMAGE9 enabled ... Skipping "
  return;
#endif

  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    return;
  }

  // TEXTURELESS
  if (texture.getPtr() == NULL)
  {
    return;
  }

  texture->setLastRenderTick(gRenderTick);

  drawImage92(0, 0, w, h, x1, y1, x2, y2, texture);
}

void pxContext::drawImage9Border(float w, float h, 
                  float /*bx1*/, float /*by1*/, float /*bx2*/, float /*by2*/,
                  float ix1, float iy1, float ix2, float iy2,
                  bool drawCenter, float* color,
                  pxTextureRef texture)
{
  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    return;
  }

  // TEXTURELESS
  if (texture.getPtr() == NULL)
  {
    return;
  }

  texture->setLastRenderTick(gRenderTick);

  //TODO - add drawImage9Border2 method
  drawImage92(0, 0, w, h, ix1, iy1, ix2, iy2, texture);
}

// convenience method
void pxContext::drawImageMasked(float x, float y, float w, float h,
                                pxConstantsMaskOperation::constants maskOp,
                                pxTextureRef t, pxTextureRef mask)
{
  this->drawImage(x, y, w, h, t , mask,
                    /* useTextureDimsAlways = */ true, /*color = */ NULL,      // DEFAULT
                    /*             stretchX = */ pxConstantsStretch::STRETCH,  // DEFAULT
                    /*             stretchY = */ pxConstantsStretch::STRETCH,  // DEFAULT
                    /*      downscaleSmooth = */ false,                        // DEFAULT
                                                 maskOp                        // PARAMETER
                    );
};

void pxContext::drawImage(float x, float y, float w, float h,
                        pxTextureRef t, pxTextureRef mask,
                        bool useTextureDimsAlways, float* color,
                        pxConstantsStretch::constants stretchX,
                        pxConstantsStretch::constants stretchY,
                        bool downscaleSmooth,
                        pxConstantsMaskOperation::constants maskOp     /* = pxConstantsMaskOperation::NORMAL */ )
{
#ifdef DEBUG_SKIP_IMAGE
#warning "DEBUG_SKIP_IMAGE enabled ... Skipping "
  return;
#endif

  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    return;
  }

  // TEXTURELESS
  if (t.getPtr() == NULL)
  {
    return;
  }

  t->setLastRenderTick(gRenderTick);

  if (mask.getPtr() != NULL)
  {
    mask->setLastRenderTick(gRenderTick);
  }

  if (stretchX < pxConstantsStretch::NONE || stretchX > pxConstantsStretch::REPEAT)
  {
    stretchX = pxConstantsStretch::NONE;
  }

  if (stretchY < pxConstantsStretch::NONE || stretchY > pxConstantsStretch::REPEAT)
  {
    stretchY = pxConstantsStretch::NONE;
  }

  float black[4] = {0,0,0,1};
  drawImageTexture(x, y, w, h, t, mask, useTextureDimsAlways,
                  color? color : black, stretchX, stretchY, maskOp);
}

void pxContext::drawDiagRect(float x, float y, float w, float h, float* color)
{
#ifdef DEBUG_SKIP_DIAG_RECT
#warning "DEBUG_SKIP_DIAG_RECT enabled ... Skipping "
   return;
#endif

  if (!mShowOutlines) return;

  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    rtLogError("cannot drawDiagRect() - width/height/gAlpha cannot be Zero.");
    return;
  }

  // COLORLESS
  if(color == NULL || color[3] == 0.0)
  {
    return;
  }

  if(boundTexture == NULL)
  {
    rtLogError("cannot drawDiagRect() on context surface because 'boundTexture' is NULL");
    return;
  }

  if (boundFramebuffer == NULL)
  {
    rtLogError("ERROR:  the 'boundFramebuffer' surface is NULL !");
    return;
  }

  /*
  const float verts[4][2] =
  {
    { x,   y   },
    { x+w, y   },
    { x+w, y+h },
    { x,   y+h },
  };
  */

  float colorPM[4];
  premultiply(colorPM, color);

  DFB_CHECK (boundFramebuffer->SetColor( boundFramebuffer, colorPM[0] * 255.0, // RGBA
                                                           colorPM[1] * 255.0,
                                                           colorPM[2] * 255.0,
                                                           colorPM[3] * 255.0 * gAlpha));

  DFB_CHECK (boundFramebuffer->DrawRectangle(boundFramebuffer, x, y, w, h));
}

void pxContext::drawOffscreen(float src_x, float src_y,
                              float dst_x, float dst_y,
                              float w,     float h,
                              pxOffscreen  &offscreen)
{
  // TRANSPARENT / DIMENSIONLESS
  if(gAlpha == 0.0 || w <= 0.0 || h <= 0.0)
  {
    return;
  }
  
  // BACKING
  if (swRasterTexture.getPtr() == NULL)
  {
    // Lazy init...
    swRasterTexture = pxSwTextureRef(new pxSwTexture());
    swRasterTexture->init(1280, 720); // HACK - hard coded for now.

   // return;
  }
  
  swRasterTexture->copy(src_x, src_y,
                        dst_x, dst_y, w, h, offscreen);
  
  pxTextureRef nullMask;
  float clear[4] = {0,0,0,0};

  pxTextureRef texture( (pxTexture *) swRasterTexture.getPtr());
  
  drawImage(dst_x, dst_y, w, h, texture, nullMask, true, clear,
            pxConstantsStretch::NONE, pxConstantsStretch::NONE);
  
  ///// CRAWL approach only
//  pxRect rect(src_x, src_y, src_x + w, src_y + h);
  pxRect rect(0,0,1280,720);
  
  swRasterTexture->clear(rect);
  offscreen.fill(pxClear);
}

void pxContext::drawDiagLine(float x1, float y1, float x2, float y2, float* color)
{
#ifdef DEBUG_SKIP_DIAG_LINE
#warning "DEBUG_SKIP_DIAG_LINE enabled ... Skipping "
   return;
#endif

  if (!mShowOutlines) return;

  if(gAlpha == 0.0)
  {
    return; // TRANSPARENT
  }

  if(color == NULL || color[3] == 0.0)
  {
    return; // COLORLESS
  }

  if(boundTexture == NULL)
  {
    rtLogError("cannot drawDiagLine() on context surface because boundTexture is NULL");
    return;
  }

  if (!boundFramebuffer)
  {
    rtLogError("ERROR:  the boundFramebuffer surface is NULL !");
  }

  /*
  const float verts[4][2] =
  {
    { x1, y1 },
    { x2, y2 },
  };
  */

  float colorPM[4];
  premultiply(colorPM, color);

  DFB_CHECK (boundFramebuffer->SetColor(boundTexture, colorPM[0] * 255.0, // RGBA
                                                      colorPM[1] * 255.0,
                                                      colorPM[2] * 255.0,
                                                      colorPM[3] * 255.0 * gAlpha));

  DFB_CHECK (boundFramebuffer->DrawLine(boundFramebuffer, x1,y1, x2,y2));
}

pxTextureRef pxContext::createTexture(pxOffscreen& o)
{
  pxTextureOffscreen* offscreenTexture = new pxTextureOffscreen(o);
  return offscreenTexture;
}

pxTextureRef pxContext::createTexture(pxOffscreen& o, const char *compressedData, size_t compressedDataSize)
{
  pxTextureOffscreen* offscreenTexture = new pxTextureOffscreen(o, compressedData, compressedDataSize);
  return offscreenTexture;
}

pxTextureRef pxContext::createTexture(float w, float h, float iw, float ih, void* buffer)
{
  pxTextureAlpha* alphaTexture = new pxTextureAlpha(w,h,iw,ih,buffer);
  return alphaTexture;
}

void pxContext::pushState()
{
  pxContextState contextState;
  contextState.matrix = gMatrix;
  contextState.alpha  = gAlpha;

  currentFramebuffer->pushState(contextState);
}

void pxContext::popState()
{
  pxContextState contextState;
  if (currentFramebuffer->popState(contextState) == PX_OK)
  {
    gAlpha  = contextState.alpha;
    gMatrix = contextState.matrix;
  }
}

void pxContext::snapshot(pxOffscreen& o)
{
  if(boundFramebuffer == NULL)
  {
    rtLogError("cannot snapshot() on context surface because boundFramebuffer is NULL");
    return;
  }

  o.init(gResW,gResH);

  void *pSrc = NULL;
  void *pDst = (void*)o.base();
  int  pitch = 0;

  boundFramebuffer->Lock(boundFramebuffer, DSLF_READ, &pSrc, &pitch);

  // READ into pxOffscreen buffer
  memcpy(pDst, pSrc, o.sizeInBytes() ); // WxH * RGBA in size

  boundFramebuffer->Unlock(boundFramebuffer);

//JUNK
#if 0
    if(boundFramebuffer)
    {
       boundFramebuffer->Dump(boundFramebuffer,
                     "/mnt/nfs/env/",
                     "screendump");
    }
#endif
//JUNK
}

void pxContext::mapToScreenCoordinates(float inX, float inY, int &outX, int &outY)
{
  pxVector4f positionVector(inX, inY, 0, 1);
  pxVector4f positionCoords = gMatrix.multiply(positionVector);

  if (positionCoords.w() == 0)
  {
    outX = positionCoords.x();
    outY = positionCoords.y();
  }
  else
  {
    outX = positionCoords.x() / positionCoords.w();
    outY = positionCoords.y() / positionCoords.w();
  }
}

void pxContext::mapToScreenCoordinates(pxMatrix4f& m, float inX, float inY, int &outX, int &outY)
{
  pxVector4f positionVector(inX, inY, 0, 1);
  pxVector4f positionCoords = m.multiply(positionVector);

  if (positionCoords.w() == 0)
  {
    outX = positionCoords.x();
    outY = positionCoords.y();
  }
  else
  {
    outX = positionCoords.x() / positionCoords.w();
    outY = positionCoords.y() / positionCoords.w();
  }
}

bool pxContext::isObjectOnScreen(float /*x*/, float /*y*/, float /*width*/, float /*height*/)
{
#if 1
  return true;
#else
  int outX1, outX2, outY1, outY2;
  mapToScreenCoordinates(width,height,outX2, outY2);
  mapToScreenCoordinates(x,y,outX1, outY1);
  int fboWidth = currentFramebuffer->width();
  int fboHeight = currentFramebuffer->height();
  if (currentFramebuffer == defaultFramebuffer)
  {
    fboWidth = gResW;
    fboHeight = gResH;
  }
  if ((outX1 < 0 && outX2 < 0) ||
      (outX1 > fboWidth && outX2 > fboWidth) ||
      (outY1 < 0 && outY2 < 0) ||
      (outY1 > fboHeight && outY2 > fboHeight))
  {
    return false;
  }
  return true;
#endif
}

void pxContext::adjustCurrentTextureMemorySize(int64_t changeInBytes, bool allowGarbageCollect)
{
  if (changeInBytes == 0)
  {
     return; // do nothing ... ever happen ?
  }

  float pc = 0.0;

  mCurrentTextureMemorySizeInBytes += changeInBytes;
  if (mCurrentTextureMemorySizeInBytes < 0)
  {
    mCurrentTextureMemorySizeInBytes = 0;
  }
  else
  {
     // Update Percentage
     pc = ((float) mCurrentTextureMemorySizeInBytes /
           (float) mTextureMemoryLimitInBytes       ) * 100.0f;
  
    // rtLogError("###  Texture Memory: %3.1f%%   (Used:  %lld bytes)  adj:  %lld bytes ", 
    //    pc, mCurrentTextureMemorySizeInBytes, changeInBytes);
  }
  //rtLogDebug("the current texture size: %" PRId64 ".", mCurrentTextureMemorySizeInBytes);
  if (mEnableTextureMemoryMonitoring && allowGarbageCollect && pc >= 100.0f)
  {
    rtLogDebug("\n ###  Texture Memory: %3.1f %%  <<<   GARBAGE COLLECT", pc);
#ifdef RUNINMAIN
    script.collectGarbage();
#else
    uv_async_send(&gcTrigger);
#endif
  }
  // else
  // {
  //     rtLogDebug("\n ###  Texture Memory: %3.2f %% ", pc);
  // }
}

void pxContext::setTextureMemoryLimit(int64_t textureMemoryLimitInBytes)
{
  mTextureMemoryLimitInBytes = textureMemoryLimitInBytes;
}

bool pxContext::isTextureSpaceAvailable(pxTextureRef texture, bool allowGarbageCollect)
{
  if (!mEnableTextureMemoryMonitoring)
    return true;

  int textureSize = (texture->width()*texture->height()*4);
  if ((textureSize + mCurrentTextureMemorySizeInBytes) >
             (mTextureMemoryLimitInBytes + mTextureMemoryLimitThresholdPaddingInBytes))
  {
    if (allowGarbageCollect)
    {
      #ifdef RUNINMAIN
          script.collectGarbage();
      #else
          uv_async_send(&gcTrigger);
      #endif
    }
    return false;
  }
  return true;
}

int64_t pxContext::currentTextureMemoryUsageInBytes()
{
  return mCurrentTextureMemorySizeInBytes;
}

int64_t pxContext::textureMemoryOverflow(pxTextureRef texture)
{
  int64_t textureSize = (texture->width()*texture->height()*4);
  int64_t availableBytes = mTextureMemoryLimitInBytes - mCurrentTextureMemorySizeInBytes;
  if (textureSize > availableBytes)
  {
    return (textureSize - availableBytes);
  }
  return 0;
}

int64_t pxContext::ejectTextureMemory(int64_t bytesRequested, bool forceEject)
{
  if (!mEnableTextureMemoryMonitoring)
    return 0;

  int64_t beforeTextureMemoryUsage = context.currentTextureMemoryUsageInBytes();
  if (!forceEject)
  {
    ejectNotRecentlyUsedTextureMemory(bytesRequested, mEjectTextureAge);
  }
  else
  {
    ejectNotRecentlyUsedTextureMemory(bytesRequested, 0);
  }
  int64_t afterTextureMemoryUsage = context.currentTextureMemoryUsageInBytes();
  return (beforeTextureMemoryUsage-afterTextureMemoryUsage);
}

pxError pxContext::setEjectTextureAge(uint32_t age)
{
  mEjectTextureAge = age;
  return PX_OK;
}

pxError pxContext::enableInternalContext(bool)
{
  return PX_OK;
}

//====================================================================================================================================================================================

#ifdef DEBUG

static void ReportSurfaceInfo(char* desc, IDirectFBSurface* surf)
{
     int w = 0, h = 0;

     DFBSurfacePixelFormat   fmt = 0;
     DFBSurfaceCapabilities caps = 0;

     rtLogWarn("\n-------------------------------------------------------------\n");
     rtLogWarn("Surface info for '%s': %p\n", desc ? desc:"(no name)", surf);

     surf->GetSize(surf, &w, &h);
     rtLogWarn("  WxH: %d x %d\n", w, h);

     surf->GetPixelFormat(surf, &fmt);
     rtLogWarn("  fmt: %d (0x%x) = %s\n", fmt, fmt, pix2str(fmt) );

     surf->GetCapabilities(surf, &caps);
     rtLogWarn(" caps: %d (0x%x) = %s\n", caps, caps, caps2str(caps) );

     rtLogWarn("\n");
}

//====================================================================================================================================================================================

// DFBSurfacePixelFormat
char* pix2str(DFBSurfacePixelFormat fmt)
{
  switch(fmt)
  {
    case DSPF_UNKNOWN:    return (char*) "DSPF_UNKNOWN";
    case DSPF_ARGB1555:   return (char*) "DSPF_ARGB1555";
    case DSPF_RGB16:      return (char*) "DSPF_RGB16";
    case DSPF_RGB24:      return (char*) "DSPF_RGB24";
    case DSPF_RGB32:      return (char*) "DSPF_RGB32";
    case DSPF_ARGB:       return (char*) "DSPF_ARGB";
    case DSPF_A8:         return (char*) "DSPF_A8";
    case DSPF_YUY2:       return (char*) "DSPF_YUY2";
    case DSPF_RGB332:     return (char*) "DSPF_RGB332";
    case DSPF_UYVY:       return (char*) "DSPF_UYVY";
    case DSPF_I420:       return (char*) "DSPF_I420";
    case DSPF_YV12:       return (char*) "DSPF_YV12";
    case DSPF_LUT8:       return (char*) "DSPF_LUT8";
    case DSPF_ALUT44:     return (char*) "DSPF_ALUT44";
    case DSPF_AiRGB:      return (char*) "DSPF_AiRGB";
    case DSPF_A1:         return (char*) "DSPF_A1";
    case DSPF_NV12:       return (char*) "DSPF_NV12";
    case DSPF_NV16:       return (char*) "DSPF_NV16";
    case DSPF_ARGB2554:   return (char*) "DSPF_ARGB2554";
    case DSPF_ARGB4444:   return (char*) "DSPF_ARGB4444";
    case DSPF_RGBA4444:   return (char*) "DSPF_RGBA4444";
    case DSPF_NV21:       return (char*) "DSPF_NV21";
    case DSPF_AYUV:       return (char*) "DSPF_AYUV";
    case DSPF_A4:         return (char*) "DSPF_A4";
    case DSPF_ARGB1666:   return (char*) "DSPF_ARGB1666";
    case DSPF_ARGB6666:   return (char*) "DSPF_ARGB6666";
    case DSPF_RGB18:      return (char*) "DSPF_RGB18";
    case DSPF_LUT2:       return (char*) "DSPF_LUT2";
    case DSPF_RGB444:     return (char*) "DSPF_RGB444";
    case DSPF_RGB555:     return (char*) "DSPF_RGB555";
    case DSPF_BGR555:     return (char*) "DSPF_BGR555";
    case DSPF_RGBA5551:   return (char*) "DSPF_RGBA5551";
    case DSPF_YUV444P:    return (char*) "DSPF_YUV444P";
    case DSPF_ARGB8565:   return (char*) "DSPF_ARGB8565";
    case DSPF_AVYU:       return (char*) "DSPF_AVYU";
    case DSPF_VYU:        return (char*) "DSPF_VYU";
    case DSPF_A1_LSB:     return (char*) "DSPF_A1_LSB";
    case DSPF_YV16:       return (char*) "DSPF_YV16";
    case DSPF_ABGR:       return (char*) "DSPF_ABGR";
    case DSPF_RGBAF88871: return (char*) "DSPF_RGBAF88871";
    case DSPF_LUT4:       return (char*) "DSPF_LUT4";
    case DSPF_ALUT8:      return (char*) "DSPF_ALUT8";
    case DSPF_LUT1:       return (char*) "DSPF_LUT1";
  }//SWITCH

  return (char *) "NOT FOUND";
}

//====================================================================================================================================================================================

static std::string strcaps;
static char *gCaps = NULL;

// DFBSurfaceCapabilities
char* caps2str(DFBSurfaceCapabilities caps)
{
  strcaps = "";

  if( (caps & DSCAPS_NONE)          == DSCAPS_NONE)          strcaps += "DSCAPS_NONE  ";
  if( (caps & DSCAPS_PRIMARY)       == DSCAPS_PRIMARY)       strcaps += "DSCAPS_PRIMARY  ";
  if( (caps & DSCAPS_SYSTEMONLY)    == DSCAPS_SYSTEMONLY)    strcaps += "DSCAPS_SYSTEMONLY  ";
  if( (caps & DSCAPS_VIDEOONLY)     == DSCAPS_VIDEOONLY)     strcaps += "DSCAPS_VIDEOONLY  ";
  if( (caps & DSCAPS_DOUBLE)        == DSCAPS_DOUBLE)        strcaps += "DSCAPS_DOUBLE  ";
  if( (caps & DSCAPS_SUBSURFACE)    == DSCAPS_SUBSURFACE)    strcaps += "DSCAPS_SUBSURFACE  ";
  if( (caps & DSCAPS_INTERLACED)    == DSCAPS_INTERLACED)    strcaps += "DSCAPS_INTERLACED  ";
  if( (caps & DSCAPS_SEPARATED)     == DSCAPS_SEPARATED)     strcaps += "DSCAPS_SEPARATED  ";
  if( (caps & DSCAPS_STATIC_ALLOC)  == DSCAPS_STATIC_ALLOC)  strcaps += "DSCAPS_STATIC_ALLOC  ";
  if( (caps & DSCAPS_TRIPLE)        == DSCAPS_TRIPLE)        strcaps += "DSCAPS_TRIPLE  ";
  if( (caps & DSCAPS_PREMULTIPLIED) == DSCAPS_PREMULTIPLIED) strcaps += "DSCAPS_PREMULTIPLIED  ";
  if( (caps & DSCAPS_DEPTH)         == DSCAPS_DEPTH)         strcaps += "DSCAPDSCAPS_DEPTHS_INTERLACED  ";
  if( (caps & DSCAPS_SHARED)        == DSCAPS_SHARED)        strcaps += "DSCAPS_SHARED  ";
  if( (caps & DSCAPS_ROTATED)       == DSCAPS_ROTATED)       strcaps += "DSCAPS_ROTATED  ";
  if( (caps & DSCAPS_ALL)           == DSCAPS_ALL)           strcaps += "DSCAPS_ALL  ";
  if( (caps & DSCAPS_FLIPPING)      == DSCAPS_FLIPPING)      strcaps += "DSCAPS_FLIPPING  ";

  if(strcaps.length() <= 1)
  {
    return (char *) "NOT FOUND";
  }

  return (gCaps = (char*) strcaps.c_str());
}

#endif // DEBUG

//====================================================================================================================================================================================

#if 0
static std::string DFBAccelerationMask2str(DFBAccelerationMask m)
{
  switch(m)
  {
     case DFXL_NONE:           return "DFXL_NONE";
     case DFXL_FILLRECTANGLE:  return "DFXL_FILLRECTANGLE";
     case DFXL_DRAWRECTANGLE:  return "DFXL_DRAWRECTANGLE";
     case DFXL_DRAWLINE:       return "DFXL_DRAWLINE";
     case DFXL_FILLTRIANGLE:   return "DFXL_FILLTRIANGLE";
     case DFXL_FILLTRAPEZOID:  return "DFXL_FILLTRAPEZOID";
     case DFXL_FILLQUADRANGLE: return "DFXL_FILLQUADRANGLE";
     case DFXL_FILLSPAN:       return "DFXL_FILLSPAN";
     case DFXL_DRAWMONOGLYPH:  return "DFXL_DRAWMONOGLYPH";
     case DFXL_BLIT:           return "DFXL_BLIT";
     case DFXL_STRETCHBLIT:    return "DFXL_STRETCHBLIT";
     case DFXL_TEXTRIANGLES:   return "DFXL_TEXTRIANGLES";
     case DFXL_BLIT2:          return "DFXL_BLIT2";
     case DFXL_TILEBLIT:       return "DFXL_TILEBLIT";
     case DFXL_DRAWSTRING:     return "DFXL_DRAWSTRING";
     case DFXL_ALL:            return "DFXL_ALL";
     case DFXL_ALL_DRAW:       return "DFXL_ALL_DRAW";
     case DFXL_ALL_BLIT:       return "DFXL_ALL_BLIT";
  }

  return "(unknown)";
}

//====================================================================================================================================================================================

static void DFBAccelerationTest(IDirectFBSurface  *dst, IDirectFBSurface  *src)
{
  rtLogDebug("\n #############  Enter DFBAccelerationTest()\n"); fflush(stdout);

  // DFBAccelerationMask ... see directfb.h

  static DFBAccelerationMask keys[] = { DFXL_NONE, DFXL_FILLRECTANGLE, DFXL_DRAWRECTANGLE, DFXL_DRAWLINE, DFXL_FILLTRIANGLE,
                                        DFXL_FILLTRAPEZOID, DFXL_FILLQUADRANGLE, DFXL_FILLSPAN, DFXL_DRAWMONOGLYPH, DFXL_BLIT,
                                        DFXL_STRETCHBLIT, DFXL_TEXTRIANGLES, DFXL_BLIT2, DFXL_TILEBLIT, DFXL_DRAWSTRING,
                                        DFXL_ALL, DFXL_ALL_DRAW, DFXL_ALL_BLIT };
  DFBAccelerationMask mask;

  dst->GetAccelerationMask( dst, src, &mask );

  int k = 0;
  do
  {
    rtLogDebug("\n ## HW:   %20s - %6s ... H/W Accelerated",
        DFBAccelerationMask2str(keys[k]).c_str(),
        ((mask & keys[k]) == keys[k]) ? "IS" : "IS NOT");
  }
  while(keys[k++] != DFXL_ALL_BLIT);

  rtLogDebug("\n\n");

  rtLogDebug("\n #############  Exit DFBAccelerationTest()\n"); fflush(stdout);
}
#endif


//====================================================================================================================================================================================
