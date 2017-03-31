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

// pxContext.h

#ifndef PX_CONTEXT_H
#define PX_CONTEXT_H

#include "rtCore.h"
#include "rtRef.h"

#include "pxCore.h"
#include "pxOffscreen.h"
#include "pxMatrix4T.h"
#include "pxConstants.h"
#include "pxTexture.h"
#include "pxContextFramebuffer.h"

#ifdef ENABLE_DFB
#include "pxContextDescDFB.h"
#else
#include "pxContextDescGL.h"
#endif //ENABLE_DFB


#define MAX_TEXTURE_WIDTH  2048
#define MAX_TEXTURE_HEIGHT 2048

#ifndef ENABLE_DFB
  #define PXSCENE_DEFAULT_TEXTURE_MEMORY_LIMIT_IN_BYTES (65 * 1024 * 1024)   // GL
  #define PXSCENE_DEFAULT_TEXTURE_MEMORY_LIMIT_THRESHOLD_PADDING_IN_BYTES (5 * 1024 * 1024)
#else
  #define PXSCENE_DEFAULT_TEXTURE_MEMORY_LIMIT_IN_BYTES (15 * 1024 * 1024)   // DFB .. Shoul be 40 ?
  #define PXSCENE_DEFAULT_TEXTURE_MEMORY_LIMIT_THRESHOLD_PADDING_IN_BYTES (5 * 1024 * 1024)
#endif

//enum pxStretch { PX_NONE = 0, PX_STRETCH = 1, PX_REPEAT = 2 };


// vec2 mean a point, use for draw polygon outline
#define MATH_TOLERANCE        2e-37f
struct Vec2 {
  float x;
  float y;

  Vec2(float _x, float _y) {
    x = _x;
    y = _y;
  }

  void normalize() {
    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f)
      return;

    n = sqrt(n);
    // Too close to zero.
    if (n < MATH_TOLERANCE)
      return;
    n = 1.0f / n;
    x *= n;
    y *= n;
  }
};

class pxContext {
 public:

  pxContext(): mShowOutlines(false), mCurrentTextureMemorySizeInBytes(0), mTextureMemoryLimitInBytes(PXSCENE_DEFAULT_TEXTURE_MEMORY_LIMIT_IN_BYTES) {}
  ~pxContext();

  void init();
  
  // debugging outlines 
  bool showOutlines() { return mShowOutlines; }
  void setShowOutlines(bool v) { mShowOutlines = v; }

  void setSize(int w, int h);
  void getSize(int& w, int& h);
  void clear(int w, int h);
  void clear(int w, int h, float *fillColor );
  void clear(int left, int top, int width, int height);
  void enableClipping(bool enable);

  void setMatrix(pxMatrix4f& m);
  pxMatrix4f getMatrix();
  void setAlpha(float a);
  float getAlpha();

  void pushState();
  void popState();

  pxContextFramebufferRef createFramebuffer(int width, int height);
  pxError updateFramebuffer(pxContextFramebufferRef fbo, int width, int height);
  pxError setFramebuffer(pxContextFramebufferRef fbo);
  pxContextFramebufferRef getCurrentFramebuffer();

  void mapToScreenCoordinates(float inX, float inY, int &outX, int &outY);
  void mapToScreenCoordinates(pxMatrix4f& m, float inX, float inY, int &outX, int &outY);
  bool isObjectOnScreen(float x, float y, float width, float height);

  pxTextureRef createTexture(); // default to use before image load is complete
  pxTextureRef createTexture(pxOffscreen& o);
  pxTextureRef createTexture(float w, float h, float iw, float ih, void* buffer, pxTextureType type = PX_TEXTURE_ALPHA);

  void snapshot(pxOffscreen& o);


  /**
   * draw rect  and support round
   * @param w  the rect width
   * @param h  the rect heifht
   * @param lineWidth  the outline width
   * @param fillColor  the fill color
   * @param lineColor the line color
   * @param radius  the rounded radius
   */
  void drawRect(float w, float h, float lineWidth, float* fillColor, float* lineColor , float radius);

  void drawImage(float x, float y, float w, float h, pxTextureRef t,
                 pxTextureRef mask, bool useTextureDimsAlways = true, float* color = NULL,
                 pxConstantsStretch::constants xStretch = pxConstantsStretch::STRETCH,
                 pxConstantsStretch::constants yStretch = pxConstantsStretch::STRETCH );

  /**
   * draw text image
   * @param x
   * @param y
   * @param w
   * @param h
   * @param t
   * @param useTextureDimsAlways
   * @param color
   * @param gradientColor
   * @param strokeColor
   */
  void drawLabelImage(float x, float y, float w, float h, pxTextureRef t, bool useTextureDimsAlways = true,
                      float *gradientColor = NULL, float *dirColor = NULL, float *strokeColor = NULL);

  /**
   * draw texture shadow
   * @param x
   * @param y
   * @param w
   * @param h
   * @param t
   * @param useTextureDimsAlways
   * @param blur
   * @param showColor
   */
  void drawTextureShadow(float x, float y, float w, float h, pxTextureRef t, bool useTextureDimsAlways = true,
                      float blur = 4, float *shadowColor = NULL);


  void drawImage9(float w, float h, float x1, float y1,
                  float x2, float y2, pxTextureRef texture);


  /**
   * draw Polygon , outline will skip if lineWidth == 0 or lineColor.a == 0 , fill will skip if fillColor.a == 0
   * @param verts the polygon verts
   * @param count  the polygon vert number
   * @param lineWidth  the polygon line width ,
   * @param fillColor the fill color
   * @param lineColor  the line color
   */
  void drawPolygon(GLfloat *verts, int count, float lineWidth, float *fillColor, float *lineColor);

  /**
   * draw lines
   * @param verts the lines verts
   * @param count  the lines vert number
   * @param lineColor the lines color
   * @param lineWidth  the lines width
   */
  void drawLines(GLfloat * verts ,int count, float * lineColor , float lineWidth);

// Only use for debug/diag purposes not for normal rendering
  void drawDiagRect(float x, float y, float w, float h, float* color);
  void drawDiagLine(float x1, float y1, float x2, float y2, float* color);
  void enableDirtyRectangles(bool enable);
  void adjustCurrentTextureMemorySize(int64_t changeInBytes);
  void setTextureMemoryLimit(int64_t textureMemoryLimitInBytes);
  bool isTextureSpaceAvailable(pxTextureRef texture);
  int64_t currentTextureMemoryUsageInBytes();
  pxError enableInternalContext(bool enable);

private:
  bool mShowOutlines;
  int64_t mCurrentTextureMemorySizeInBytes;
  int64_t mTextureMemoryLimitInBytes;
};


#endif
