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

// pxFont.cpp

#include "rtFileDownloader.h"
#include "pxFont.h"
#include "pxTimer.h"
#include "pxText.h"
#include FT_BITMAP_H

#include <math.h>
#include <map>

using namespace std;

struct GlyphKey 
{
  uint32_t mFontId;
  uint32_t mPixelSize;
  uint32_t mCodePoint;
  uint32_t mOutlineSize;
  bool mBold;
  bool mItalic;
  bool mShadow;
  uint32_t mShadowBlurRadio;

  // Provide a "<" operator that orders keys.
  // The way it orders them doesn't matter, all that matters is that
  // it orders them consistently.
  bool operator<(GlyphKey const& other) const {
    if (mFontId < other.mFontId) return true; else
      if (mFontId == other.mFontId) {
        if (mPixelSize < other.mPixelSize) return true; else
          if (mPixelSize == other.mPixelSize) {
            if (mCodePoint < other.mCodePoint) return true; else
              if (mCodePoint == other.mCodePoint) {
                if (mOutlineSize < other.mOutlineSize) return true; else
                  if (mOutlineSize == other.mOutlineSize) {
                    if (mBold != other.mBold) return mBold; else
                      if (mItalic != other.mItalic) return mItalic; else
                        if (mShadow != other.mShadow) return mShadow; else
                          if (mShadowBlurRadio < other.mShadowBlurRadio) 
                            return true;
                  }
              }
          }
      }
    return false;
  }
};

typedef map<GlyphKey,GlyphCacheEntry*> GlyphCache;

GlyphCache gGlyphCache;


#include "pxContext.h"

extern pxContext context;

#if 1
// TODO can we eliminate direct utf8.h usage
extern "C" {
#include "utf8.h"
}
#endif

#ifndef MIN
#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#endif  // MIN

#ifndef MAX
#define MAX(x,y) (((x) < (y)) ? (y) : (x))
#endif  // MAX


FT_Library ft;
uint32_t gFontId = 0;

const float BOLD_ADD_RATE = 0.04f;
const float BOLD_ADD_RATE_YX = 0.6f;
const float ITALIC_ADD_RATE = 0.21f;

pxFont::pxFont(rtString fontUrl)
:pxResource()
,mPixelSize(0) 
,mFontData(0)
,mOutlineSize(0)
,mItalic(false)
,mBold(false)
,mShadow(false)
,mShadowBlurRadio(0)
{  
  mFontId = gFontId++; 
  mUrl = fontUrl;
  mStroker = 0;
  mShadowOffsetX = 0;
  mShadowOffsetY = 0;
}

pxFont::~pxFont() 
{
  rtLogInfo("~pxFont %s\n", mUrl.cString());
  gUIThreadQueue.removeAllTasksForObject(this);
  // download should be canceled/removed in pxResource
  //if (mDownloadRequest != NULL)
  //{
    //// clear any pending downloads
    //mFontDownloadRequest->setCallbackFunctionThreadSafe(NULL);
  //}  
   
  pxFontManager::removeFont( mUrl);
 
  if ( mInitialized) 
  {
    FT_Done_Face(mFace);
  }
  mFace = 0;

  if (mStroker) 
  {
    FT_Stroker_Done(mStroker);
    mStroker = 0; 
  }
  
  if (mFontData) {
    free(mFontData);
    mFontData = 0;
  } 
   
}
bool pxFont::loadResourceData(rtFileDownloadRequest* fileDownloadRequest)
{
      // Load the font data
      init( (FT_Byte*)fileDownloadRequest->downloadedData(), 
            (FT_Long)fileDownloadRequest->downloadedDataSize(), 
            fileDownloadRequest->fileUrl().cString());
            
      return true;
}

void pxFont::loadResourceFromFile()
{
    rtError e = init(mUrl);
    if (e != RT_OK)
    {
      rtLogWarn("Could not load font face %s\n", mUrl.cString());
      mLoadStatus.set("statusCode", PX_RESOURCE_STATUS_FILE_NOT_FOUND);
      // Since this object can be released before we get a async completion
      // We need to maintain this object's lifetime
      // TODO review overall flow and organization
      AddRef();     
      gUIThreadQueue.addTask(onDownloadCompleteUI, this, (void*)"reject");
    }
    else
    {
      mLoadStatus.set("statusCode", PX_RESOURCE_STATUS_OK);
      // Since this object can be released before we get a async completion
      // We need to maintain this object's lifetime
      // TODO review overall flow and organization
      AddRef();      
      gUIThreadQueue.addTask(onDownloadCompleteUI, this, (void*)"resolve");

    } 
}

rtError pxFont::init(const char* n)
{
  mUrl = n;
  if (FT_New_Face(ft, n, 0, &mFace))
    return RT_FAIL;
  mInitialized = true;
  setPixelSize(defaultPixelSize);

  return RT_OK;
}

rtError pxFont::init(const FT_Byte*  fontData, FT_Long size, const char* n,FT_Long outlineSize)
{
  // We need to keep a copy of fontData since the download will be deleted.
  mFontData = (char *)malloc(size);
  memcpy(mFontData, fontData, size);
  
  if (FT_New_Memory_Face(ft, (const FT_Byte*)mFontData, size, 0, &mFace))
    return RT_FAIL;

      // set the requested font size
  mUrl = n;
  mInitialized = true;
  setOutlineSize(outlineSize);
  setPixelSize(defaultPixelSize);
  
  return RT_OK;
}


void pxFont::setOutlineSize(uint32_t size )
{

  if (size != mOutlineSize)
  {
    mOutlineSize = size;
    if (mStroker)
    {
      FT_Stroker_Done(mStroker);
      mStroker = 0; 
    } 
    if (mOutlineSize > 0)
    {
      FT_Stroker_New(ft, &mStroker);
      FT_Stroker_Set(mStroker,
          (int)(mOutlineSize * 32),
          FT_STROKER_LINECAP_ROUND,
          FT_STROKER_LINEJOIN_ROUND,
          0);
    }   
  }
}

void pxFont::setItalic(bool var)
{
  mItalic = var;
}

void pxFont::setBold(bool var)
{
  mBold = var;
}

void pxFont::setShadow(bool var, float shadowColor[], uint32_t blurRadio, float offset[])
{
  mShadow = var;
  mShadowColor[0] = shadowColor[0];
  mShadowColor[1] = shadowColor[1];
  mShadowColor[2] = shadowColor[2];
  mShadowColor[3] = shadowColor[3];
  
  mShadowBlurRadio = blurRadio;

  mShadowOffsetX = offset[0];
  mShadowOffsetY = offset[1];
}

void pxFont::setPixelSize(uint32_t s)
{
  if (mPixelSize != s && mInitialized)
  {
    int dpi = 72;
    int fontSizePoints = (int)(64.f * s);
    FT_Set_Char_Size(mFace, fontSizePoints, fontSizePoints, dpi, dpi);

    mPixelSize = s;
  }
}
void pxFont::getHeight(uint32_t size, float& height)
{
	// TO DO:  check FT_IS_SCALABLE 
  if ( !mInitialized) 
  {
    rtLogWarn("getHeight called on font before it is initialized\n");
    return;
  }
  
  setPixelSize(size);
  
	FT_Size_Metrics* metrics = &mFace->size->metrics;
  	
	height = (metrics->height>>6) + mOutlineSize; 
}
  
void pxFont::getMetrics(uint32_t size, float& height, float& ascender, float& descender, float& naturalLeading)
{
	// TO DO:  check FT_IS_SCALABLE 
  if ( !mInitialized) 
  {
    rtLogWarn("Font getMetrics called on font before it is initialized\n");
    return;
  }
  if (!size) 
  {
    rtLogWarn("Font getMetrics called with pixelSize=0\n");
  }
  
  setPixelSize(size);
  
	FT_Size_Metrics* metrics = &mFace->size->metrics;
  	
	height = (metrics->height>>6) + mOutlineSize;
	ascender = metrics->ascender>>6; 
	descender = - metrics->descender>>6; 
  naturalLeading = height - (ascender + descender);
}

unsigned char * pxFont::getGlyphBitmapWithOutline(unsigned short theChar, FT_BBox &bbox)
{   
  unsigned char* ret = nullptr;
  if (FT_Load_Char(mFace, theChar, FT_LOAD_NO_BITMAP) == 0)
  {
    applyItalic(mFace->glyph);
    if (mFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
    {
      FT_Glyph glyph;
      if (FT_Get_Glyph(mFace->glyph, &glyph) == 0)
      {
        FT_Glyph_StrokeBorder(&glyph, mStroker, 0, 1);
        if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
        {
          FT_Outline* outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
          if (mBold) {
            uint32_t k = (uint32_t)(mPixelSize*BOLD_ADD_RATE+1);
            k = (k%2)?(k+1):(k); // will add px
            FT_Pos xBold = k << 6; 
            FT_BBox oldBox;
            FT_Outline_Get_CBox(outline, &oldBox);
            FT_Outline_EmboldenXY(outline, xBold, xBold*BOLD_ADD_RATE_YX);
          }

          FT_Glyph_Get_CBox(glyph,FT_GLYPH_BBOX_GRIDFIT,&bbox);

          long width = (bbox.xMax - bbox.xMin)>>6;
          long rows = (bbox.yMax - bbox.yMin)>>6;

          FT_Bitmap bmp;
          bmp.buffer = new unsigned char[width * rows];
          memset(bmp.buffer, 0, width * rows);
          bmp.width = (int)width;
          bmp.rows = (int)rows;
          bmp.pitch = (int)width;
          bmp.pixel_mode = FT_PIXEL_MODE_GRAY;
          bmp.num_grays = 256;

          FT_Raster_Params params;
          memset(&params, 0, sizeof (params));
          params.source = outline;
          params.target = &bmp;
          params.flags = FT_RASTER_FLAG_AA;
          FT_Outline_Translate(outline,-bbox.xMin,-bbox.yMin);
          FT_Outline_Render(ft, outline, &params);

          ret = bmp.buffer;
        }
        FT_Done_Glyph(glyph);
      }
    }
  }
  return ret;
}


rtError pxFont::applyBold(uint32_t &offsetX, uint32_t &offsetY)
{
  FT_Pos xBold, yBold;

  if (mBold)
  {
    uint32_t k = (uint32_t)(mPixelSize*BOLD_ADD_RATE+1);
    k = (k%2)?(k+1):(k); // will add px
    xBold = k * 64; 
    yBold = xBold * BOLD_ADD_RATE_YX;
  }
  else
  {
    xBold = 0; 
    yBold = 0;
  }

  FT_GlyphSlot g = mFace->glyph;
  if (g->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_BBox oldBox;
    FT_Outline_Get_CBox(&(g->outline), &oldBox);
    FT_Outline_EmboldenXY(&(g->outline), xBold, yBold);
    FT_BBox newBox;
    FT_Outline_Get_CBox(&(g->outline), &newBox);
    xBold = (newBox.xMax - newBox.xMin) - (oldBox.xMax - oldBox.xMin);
    yBold = (newBox.yMax - newBox.yMin) - (oldBox.yMax - oldBox.yMin);
    offsetX = xBold;
    offsetY = yBold;
  }
  else if (g->format == FT_GLYPH_FORMAT_BITMAP)
  {
    FT_Bitmap_Embolden(ft, &(g->bitmap), xBold, yBold);
    offsetX = xBold;
    offsetY = yBold;
  }
  return RT_OK;
}

rtError pxFont::applyItalic(FT_GlyphSlot& g)
{
  FT_Matrix matrix;
  matrix.xx = 0x10000L;
  matrix.xy = 0;
  matrix.yx = 0;
  matrix.yy = 0x10000L;
  if (mItalic)
  {
    matrix.xy = ITALIC_ADD_RATE * 0x10000L;
  }

  if (g->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_Outline_Transform(&g->outline, &matrix);
  }
  else
  {
    FT_Set_Transform( mFace, &matrix, 0 );  
  }
  return RT_OK;
}


unsigned char* pxFont::applyShadow(GlyphCacheEntry* entry, unsigned char* data, uint32_t &outW, uint32_t &outH)
{  
  if ((mShadow or mOutlineSize > 0 ) && outW > 0 && outH > 0 ) 
  {
    uint32_t externPixels = 0;
    if (mShadow)
    {
      externPixels = mShadowBlurRadio;
    }
    else
    {
      externPixels = 4;
    }

    uint32_t realOutW = outW + externPixels * 2;
    uint32_t realOutH = outH + externPixels * 2;
    long index, index2;
    uint32_t bitSize = 1;
    if (mOutlineSize > 0)
    {
      bitSize = 2;
    }
    unsigned char* blendImage = (unsigned char *) malloc(sizeof(unsigned char ) * realOutW * realOutH * bitSize);

    if (!blendImage) {
      return data;
    }
    memset(blendImage, 0, realOutW * realOutH * bitSize);
    uint32_t px = externPixels;
    uint32_t py = externPixels;
    for (long x = 0; x < outW; ++x)
    {
      for (long y = 0; y < outH; ++y)
      {
        index = px + x + ((py + y) * realOutW);
        index2 = x + (y * outW);
        if (bitSize == 2 ) 
        {
          blendImage[2*index] = data[2*index2];
          blendImage[2*index+1] = data[2*index2+1];
        }
        else{
          blendImage[index] = data[index2]; 
        }
      }
    }

    outW = realOutW;
    outH = realOutH;
    entry->bitmap_top += externPixels;
    return blendImage;
  }
  return data;
}


const GlyphCacheEntry *pxFont::getGlyph(uint32_t codePoint) 
{
  GlyphKey key;
  key.mFontId = mFontId;
  key.mBold = mBold;
  key.mItalic = mItalic;
  key.mPixelSize = mPixelSize;
  key.mCodePoint = codePoint;
  key.mOutlineSize = mOutlineSize;
  key.mShadow = mShadow;
  key.mShadowBlurRadio = mShadowBlurRadio;

  GlyphCache::iterator it = gGlyphCache.find(key);
  if (it != gGlyphCache.end()) 
  {
    return it->second;
  } 
  else 
  {
    //load bitmap left
    if (FT_Load_Char(mFace, codePoint, FT_LOAD_RENDER))
    {
      return NULL;
    }
    else
    {
      GlyphCacheEntry *entry = new GlyphCacheEntry;
      uint32_t nBitmapLeft = mFace->glyph->bitmap_left;

      FT_Load_Char(mFace, codePoint, FT_LOAD_NO_BITMAP);
      FT_GlyphSlot g = mFace->glyph;

      uint32_t offsetX = 0, offsetY = 0;
      applyBold(offsetX, offsetY);

      rtLogDebug("glyph cache miss");

      uint32_t outWidth, outHeight;
      applyItalic(g);

      if (FT_Render_Glyph(g, FT_RENDER_MODE_NORMAL)) 
      {
        return NULL;
      }
      unsigned char *ret = g->bitmap.buffer;

      outWidth = g->bitmap.width;
      outHeight = g->bitmap.rows;

      entry->bitmap_left = g->bitmap_left;
      entry->bitmap_top = g->bitmap_top;
      entry->advancedotx = g->advance.x + offsetX;
      entry->advancedoty = g->advance.y + offsetY;
      entry->vertAdvance = g->metrics.vertAdvance + offsetX; // !CLF: Why vertAdvance? SHould only be valid for vert layout of text.

      if (mItalic && (codePoint == 'U'))
      {
        entry->bitmap_left = nBitmapLeft;
      }

      if (codePoint == 58 or codePoint == 59)
      {
        entry->bitmap_left *= 0.5;
      }

      if (mOutlineSize > 0)
      {
        uint32_t theChar = codePoint;

        unsigned char* copyBitmap = (unsigned char *) malloc( sizeof(unsigned char ) * outWidth * outHeight);
        memcpy(copyBitmap,ret,outWidth * outHeight * sizeof(unsigned char));

        FT_BBox bbox;
        unsigned char* outlineBitmap = getGlyphBitmapWithOutline(theChar,bbox);
        if (outlineBitmap == nullptr)
        {
            ret = nullptr;
            free(copyBitmap);
            return NULL;
        }

        long x = g->metrics.horiBearingX >> 6;
        long y = -(g->metrics.horiBearingY >> 6);
        long width = (g->metrics.width >> 6);
        long height = (g->metrics.height >> 6);

        long glyphMinX = x;
        long glyphMaxX = x + outWidth;
        long glyphMinY = -outHeight - y;
        long glyphMaxY = -y;

        long outlineMinX = bbox.xMin >> 6;
        long outlineMaxX = bbox.xMax >> 6;
        long outlineMinY = bbox.yMin >> 6;
        long outlineMaxY = bbox.yMax >> 6;

        long outlineWidth = outlineMaxX - outlineMinX;
        long outlineHeight = outlineMaxY - outlineMinY;

        long blendImageMinX = MIN(outlineMinX, glyphMinX);
        long blendImageMaxY = MAX(outlineMaxY, glyphMaxY);
        long blendWidth = MAX(outlineMaxX, glyphMaxX) - blendImageMinX;
        long blendHeight = blendImageMaxY - MIN(outlineMinY, glyphMinY);

        float outlineScale = 0.5;
        long outLineOffset = floor(mOutlineSize * outlineScale);
        x = blendImageMinX;
        y = -blendImageMaxY + outLineOffset;

        long index, index2;
        unsigned char* blendImage = (unsigned char*) malloc(sizeof(unsigned char) * blendWidth * blendHeight * 2);
        memset(blendImage, 0, blendWidth * blendHeight * 2);

        long px = outlineMinX - blendImageMinX;
        long py = blendImageMaxY - outlineMaxY;

        for (long x = 0; x < outlineWidth; ++x)
        {
          for (long y = 0; y < outlineHeight; ++y)
          {
            index = px + x + ((py + y) * blendWidth);
            index2 = x + (y * outlineWidth);
            blendImage[2 * index] = outlineBitmap[index2];
          }
        }

        px = (long)(px + outLineOffset);
        if (px + outWidth > blendWidth) 
        {
          px = blendWidth - outWidth;
        }

        py = (long)(py + outLineOffset);
        if (py + outHeight > blendHeight) 
        {
          py = blendHeight - outHeight;
        }
        for (long x = 0; x < outWidth; ++x)
        {
          for (long y = 0; y < outHeight; ++y)
          {
            index = px + x + ((y + py) * blendWidth);
            index2 = x + (y * outWidth);
            blendImage[2 * index + 1] = copyBitmap[index2];
          }
        }

        width = blendWidth;
        height = blendHeight;
        outWidth = blendWidth;
        outHeight = blendHeight;

        free(outlineBitmap);
        free(copyBitmap);
        ret = blendImage;

        unsigned char *newBmp = applyShadow(entry, ret, outWidth, outHeight);
        if (newBmp != ret) 
        {
          free(ret);
          ret = newBmp;
        }

        entry->mTexture = context.createTexture(outWidth, outHeight,
                                                outWidth, outHeight,
                                                ret, PX_TEXTURE_ALPHA_88);
      } 
      else 
      {
        ret = applyShadow(entry, ret, outWidth, outHeight);
        entry->mTexture = context.createTexture(outWidth, outHeight,
                                                outWidth, outHeight,
                                                ret, PX_TEXTURE_ALPHA);
      }

      entry->bitmapdotwidth = outWidth;
      entry->bitmapdotrows = outHeight;

      gGlyphCache.insert(make_pair(key, entry));
      return entry;
    }
  }
}

void pxFont::measureTextInternal(const char *text, uint32_t size, float sx, float sy,
                                 float &w, float &h) 
{
  if (!mInitialized) 
  {
    rtLogWarn("measureText called TOO EARLY -- not initialized or font not loaded!\n");
    return;
  }

  setPixelSize(size);

  w = 0;
  h = 0;
  if (!text)
    return;

  int i = 0;
  u_int32_t codePoint;

  FT_Size_Metrics *metrics = &mFace->size->metrics;

  h = (metrics->height >> 6) + mOutlineSize;
  float lw = 0;
  float lastCodeWidth = 0;

  while ((codePoint = u8_nextchar((char *) text, &i)) != 0) 
  {
    const GlyphCacheEntry *entry = getGlyph(codePoint);
    if (!entry)
      continue;

    if (codePoint != '\n') 
    {
      lastCodeWidth = (entry->advancedotx >> 6) * sx;
      lw += lastCodeWidth;
    }
    else
    {
      h += (metrics->height>>6) + mOutlineSize;
      lw = 0;
    }
    w = pxMax<float>(w, lw);
  }
  if (mItalic)
  {
    w += lastCodeWidth;
  }
  h *= sy;
}


void pxFont::renderText(const char *text, uint32_t size, float x, float y, float sx, float sy, float *color, float mw,
                        float *gradientColor, float *strokeColor, float * dropShadowColor, float strokeWidth, bool italic, bool bold,
                        bool dropShadow, float dropShadowOffsetX, float dropShadowOffsetY, float dropShadowBlur) {
  if (!text || !mInitialized) 
  {
    rtLogWarn("renderText called on font before it is initialized\n");
    return;
  }


  int i = 0;
  u_int32_t codePoint;

  setPixelSize(size);
  setBold(bold);
  setItalic(italic);
  setOutlineSize(strokeWidth);
  if (dropShadow == false || dropShadowColor == NULL || dropShadowColor[3] <= 0.01) 
  {
    float z[4] = {0, 0, 0, 0};
    setShadow(false, z, 0, z);
  } 
  else 
  {
    float offset[2] = {dropShadowOffsetX,dropShadowOffsetY};
    setShadow(dropShadow,dropShadowColor,dropShadowBlur,offset);
  }

  FT_Size_Metrics *metrics = &mFace->size->metrics;
  float lineHeight = ((metrics->height >> 6) + strokeWidth) * sy;

  while ((codePoint = u8_nextchar((char *) text, &i)) != 0) 
  {
    const GlyphCacheEntry *entry = getGlyph(codePoint);
    if (!entry)
      continue;
    float bitmapTop = entry->bitmap_top * sy;
    float x2 = x + entry->bitmap_left * sx;
    float y2 = (y - bitmapTop - strokeWidth *sy) + (metrics->ascender >> 6);
    float w = entry->bitmapdotwidth * sx;
    float h = entry->bitmapdotrows * sy;

    if (codePoint != '\n') 
    {
      if (x == 0) 
      {
        float c[4] = {0, 1, 0, 1};
        context.drawDiagLine(0, y + (metrics->ascender >> 6), mw,
                             y + (metrics->ascender >> 6), c);
      }
      pxTextureRef texture = entry->mTexture;
      pxTextureRef nullImage;

      if (mShadow) 
      {  //render shadow first
        float shadowX = x2 + mShadowOffsetX;
        float shadowY = y2 + mShadowOffsetY;
        context.drawTextureShadow(shadowX, shadowY, w, h, texture, false, mShadowBlurRadio, mShadowColor);
      }
      
      context.drawLabelImage(x2, y2, w, h, texture, bitmapTop, lineHeight, false, color, gradientColor, strokeColor);
      x += (entry->advancedotx >> 6) * sx;
      // no change to y because we are not moving to next line yet
    } 
    else 
    {
      x = 0;
      // Use height to advance to next line
      y += lineHeight;
    }
  }
}

void pxFont::measureTextChar(u_int32_t codePoint, uint32_t size,  float sx, float sy, 
                         float& w, float& h) 
{

  if ( !mInitialized) 
  {
    rtLogWarn("measureTextChar called TOO EARLY -- not initialized or font not loaded!\n");
    return;
  }
    
  setPixelSize(size);

  
  w = 0; h = 0;
  
  FT_Size_Metrics* metrics = &mFace->size->metrics;
  
  h = metrics->height>>6;
  float lw = 0;

  const GlyphCacheEntry* entry = getGlyph(codePoint);
  if (!entry) 
    return;


  lw = (entry->advancedotx >> 6) * sx;

  w = pxMax<float>(w, lw);

  h += h == 0 ? 0 : mOutlineSize;

  h *= sy;
}


/*
#### getFontMetrics - returns information about the font (font and size).  It does not convey information about the text of the font.  
* See section 3.a in http://www.freetype.org/freetype2/docs/tutorial/step2.html .  
* The returned object has the following properties:
* height - float - the distance between baselines
* ascent - float - the distance from the baseline to the font ascender (note that this is a hint, not a solid rule)
* descent - float - the distance from the baseline to the font descender  (note that this is a hint, not a solid rule)
*/
rtError pxFont::getFontMetrics(uint32_t pixelSize, rtObjectRef& o) 
{
  //rtLogDebug("pxFont::getFontMetrics\n");  
	float height, ascent, descent, naturalLeading;
	pxTextMetrics* metrics = new pxTextMetrics();

  if (!mInitialized ) 
  {
    rtLogWarn("getFontMetrics called TOO EARLY -- not initialized or font not loaded!\n");
    o = metrics;
    return RT_OK; // !CLF: TO DO - COULD RETURN RT_ERROR HERE TO CATCH NOT WAITING ON PROMISE
  }

	getMetrics(pixelSize, height, ascent, descent, naturalLeading);
	metrics->setHeight(height);
	metrics->setAscent(ascent);
	metrics->setDescent(descent);
  metrics->setNaturalLeading(naturalLeading);
  metrics->setBaseline(ascent);
	o = metrics;

	return RT_OK;
}

/** Public API exposed to java script */
rtError pxFont::measureText(uint32_t pixelSize, rtString stringToMeasure, rtObjectRef& o)
{
  pxTextSimpleMeasurements* measure = new pxTextSimpleMeasurements();
  
  if (!mInitialized ) 
  {
    rtLogWarn("measureText called TOO EARLY -- not initialized or font not loaded!\n");
    o = measure;
    return RT_OK; // !CLF: TO DO - COULD RETURN RT_ERROR HERE TO CATCH NOT WAITING ON PROMISE
  } 
  
  if (!pixelSize) 
  {
    rtLogWarn("Font measureText called with pixelSize=0\n");
  }    
  
  float w, h;
  measureTextInternal(stringToMeasure, pixelSize, 1.0,1.0, w, h);
  //rtLogDebug("pxFont::measureText returned %f and %f for pixelSize=%d and text \"%s\"\n",w, h,pixelSize, stringToMeasure.cString());

  measure->setW(w);
  measure->setH(h);
  o = measure;
  return RT_OK; 
}


/**********************************************************************/
/**                    pxFontManager                                  */
/**********************************************************************/
FontMap pxFontManager::mFontMap;
bool pxFontManager::init = false;
void pxFontManager::initFT() 
{
  if (init) 
  {
    //rtLogDebug("initFT returning; already inited\n");
    return;
  }
  init = true;
  
  if (FT_Init_FreeType(&ft)) 
  {
    rtLogError("Could not init freetype library\n");
    return;
  }
  
}
rtRef <pxFont> pxFontManager::getFont(const char *url) 
{
  initFT();

  rtRef <pxFont> pFont;

  if (!url || !url[0]) 
  {
    url = defaultFont;
  } 
  else 
  {
    FILE* fp = nullptr;
    string urlString = string(url);
    string newLocalTTF = string("fonts/") + urlString;
    char* ttf = strchr(url, '.ttf');
    if (ttf == NULL)
    {
      newLocalTTF.append(".ttf");
    }

    FontMap::iterator it = mFontMap.find(newLocalTTF.c_str());
    if (it != mFontMap.end())  // local key search in map
    {   
      rtLogDebug("Found pxFont in map for %s\n", url);
      pFont = it->second;
      return pFont;
    } 
    else 
    {
      fp = fopen(newLocalTTF.c_str(), "rb");
      rtLogInfo("start find local font = %s.", newLocalTTF.c_str());
      if (fp != nullptr) {
        rtLogInfo("found font %s success.", newLocalTTF.c_str());
        url = newLocalTTF.c_str();
        fclose(fp);
      } 
      else 
      {
        rtLogInfo("Can not find the font = %s.", newLocalTTF.c_str());
      }
    }
  }

  FontMap::iterator it = mFontMap.find(url);
  if (it != mFontMap.end()) 
  {
    rtLogDebug("Found pxFont in map for %s\n", url);
    pFont = it->second;
    return pFont;
  } 
  else 
  {
    rtLogDebug("Create pxFont in map for %s\n", url);
    pFont = new pxFont(url);
    mFontMap.insert(make_pair(url, pFont));
    pFont->loadResource();
  }

  return pFont;
}


void pxFontManager::removeFont(rtString fontName)
{
  FontMap::iterator it = mFontMap.find(fontName);
  if (it != mFontMap.end())
  {  
    mFontMap.erase(it);
  }
}

void pxFontManager::clearAllFonts()
{
  for (GlyphCache::iterator it =  gGlyphCache.begin(); it != gGlyphCache.end(); it++)
  {
    it->second->mTexture = NULL;
    delete it->second;
  }
  gGlyphCache.clear();
}

// pxTextMetrics
rtDefineObject(pxTextMetrics, pxResource);
rtDefineProperty(pxTextMetrics, height); 
rtDefineProperty(pxTextMetrics, ascent);
rtDefineProperty(pxTextMetrics, descent);
rtDefineProperty(pxTextMetrics, naturalLeading);
rtDefineProperty(pxTextMetrics, baseline);

// pxFont
rtDefineObject(pxFont, pxResource);
rtDefineMethod(pxFont, getFontMetrics);
rtDefineMethod(pxFont, measureText);

rtDefineObject(pxTextSimpleMeasurements, pxResource);
rtDefineProperty(pxTextSimpleMeasurements, w);
rtDefineProperty(pxTextSimpleMeasurements, h);
