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
  bool mBold;
  bool mItalic;

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
                if (mBold != other.mBold) return mBold; else
                  if (mItalic != other.mItalic) return mItalic; else
                    return true;
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

FT_Library ft;
uint32_t gFontId = 0;

const float BOLD_ADD_RATE = 0.04f;
const float BOLD_ADD_RATE_YX = 0.6f;
const float ITALIC_ADD_RATE = 0.21f;

pxFont::pxFont(rtString fontUrl)
:pxResource()
,mPixelSize(0) 
,mFontData(0)
,mItalic(false)
,mBold(false)
{  
  mFontId = gFontId++; 
  mUrl = fontUrl;

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
 
  if( mInitialized) 
  {
    FT_Done_Face(mFace);
  }
  mFace = 0;
  
  if(mFontData) {
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
    
  if(FT_New_Face(ft, n, 0, &mFace))
    return RT_FAIL;
  
  mInitialized = true;
  setPixelSize(defaultPixelSize);

  return RT_OK;
}

rtError pxFont::init(const FT_Byte*  fontData, FT_Long size, const char* n)
{
  // We need to keep a copy of fontData since the download will be deleted.
  mFontData = (char *)malloc(size);
  memcpy(mFontData, fontData, size);
  
  if(FT_New_Memory_Face(ft, (const FT_Byte*)mFontData, size, 0, &mFace))
    return RT_FAIL;

  mUrl = n;
  mInitialized = true;
  setPixelSize(defaultPixelSize);
  
  return RT_OK;
}


void pxFont::setItalic(bool var)
{
  mItalic = var;
}

void pxFont::setBold(bool var)
{
  mBold = var;
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
  if( !mInitialized) 
  {
    rtLogWarn("getHeight called on font before it is initialized\n");
    return;
  }
  
  setPixelSize(size);
  
	FT_Size_Metrics* metrics = &mFace->size->metrics;
  	
	height = metrics->height>>6; 
}
  
void pxFont::getMetrics(uint32_t size, float& height, float& ascender, float& descender, float& naturalLeading)
{
	// TO DO:  check FT_IS_SCALABLE 
  if( !mInitialized) 
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
  	
	height = metrics->height>>6;
	ascender = metrics->ascender>>6; 
	descender = -metrics->descender>>6; 
  naturalLeading = height - (ascender + descender);
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
const GlyphCacheEntry* pxFont::getGlyph(uint32_t codePoint)
{
  GlyphKey key; 
  key.mFontId = mFontId; 
  key.mBold = mBold;
  key.mItalic = mItalic;
  key.mPixelSize = mPixelSize; 
  key.mCodePoint = codePoint;
  GlyphCache::iterator it = gGlyphCache.find(key);
  if (it != gGlyphCache.end())
  {
    return it->second;
  } 
  else
  {
    if(FT_Load_Char(mFace, codePoint, FT_LOAD_RENDER))
    {
      return NULL;
    }
    else
    {
      rtLogDebug("glyph cache miss");
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
      
      entry->mTexture = context.createTexture(outWidth, outHeight,
                                              outWidth, outHeight,
                                              g->bitmap.buffer);

      entry->bitmapdotwidth = outWidth;
      entry->bitmapdotrows = outHeight;
      
      gGlyphCache.insert(make_pair(key,entry));
      return entry;
    }
  }
  return NULL;
}

void pxFont::measureTextInternal(const char* text, uint32_t size,  float sx, float sy, 
                         float& w, float& h) 
{
  if( !mInitialized) 
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
  
  FT_Size_Metrics* metrics = &mFace->size->metrics;
  
  h = metrics->height>>6;
  float lw = 0;
  float lastCodeWidth = 0;
  while((codePoint = u8_nextchar((char*)text, &i)) != 0) 
  {
    const GlyphCacheEntry* entry = getGlyph(codePoint);
    if (!entry) 
      continue;
      
    if (codePoint != '\n')
    {
      lastCodeWidth = (entry->advancedotx >> 6) * sx;
      lw += lastCodeWidth;
    }
    else
    {
      h += (metrics->height>>6) *sy;
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
                        bool italic, bool bold) 
{
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
  FT_Size_Metrics* metrics = &mFace->size->metrics;
  float lineHeight = (metrics->height >> 6) * sy;

  while((codePoint = u8_nextchar((char*)text, &i)) != 0) 
  {
    const GlyphCacheEntry* entry = getGlyph(codePoint);
    if (!entry) 
      continue;
    float bitmapTop = entry->bitmap_top * sy;
    float x2 = x + entry->bitmap_left * sx;
    float y2 = (y - bitmapTop) + (metrics->ascender>>6);
    float w = entry->bitmapdotwidth * sx;
    float h = entry->bitmapdotrows * sy;
    
    if (codePoint != '\n')
    {
      if (x == 0) 
      {
        float c[4] = {0, 1, 0, 1};
        context.drawDiagLine(0, y+(metrics->ascender>>6), mw, 
                             y+(metrics->ascender>>6), c);
      }
      
      pxTextureRef texture = entry->mTexture;
      pxTextureRef nullImage;
      context.drawLabelImage(x2, y2, w, h, texture, bitmapTop, lineHeight, false, color);
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
  if (!mInitialized) 
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
rtRef<pxFont> pxFontManager::getFont(const char* url)
{
  initFT();

  rtRef<pxFont> pFont;

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
