/**
 * pxGraphic to render polygon, lines
 */
#ifndef PX_GRAPHIC_H
#define PX_GRAPHIC_H
#include <vector>
#include "pxScene2d.h"

using std::vector;

class pxGraphic: public pxObject {
public:
  rtDeclareObject(pxGraphic, pxObject);
  rtProperty(fillColor, fillColor, setFillColor, uint32_t);
  rtProperty(lineColor, lineColor, setLineColor, uint32_t);
  rtProperty(lineWidth, lineWidth, setLineWidth, float);
  rtProperty(type, type , setType , rtString);
  rtProperty(points, points, setPoints , rtObjectRef);

  pxGraphic(pxScene2d *scene);

  virtual void onInit() {mReady.send("resolve",this);}


  rtError points(rtObjectRef & /*pts*/) const { return RT_OK; }

  rtError setPoints(rtObjectRef pts) {
    this->mVertNumber = 0;
    uint32_t len = pts.get<uint32_t>("length");
    for (uint32_t i = 0; i < len; i++) {
      float v = pts.get<float>(i);
      this->mVerts[i] = v;
    }
    this->mVertNumber = len/2;
    return RT_OK;
  }

  rtError type(rtString & /*type*/) const { return RT_OK; }

  rtError setType(rtString t) {
    strcpy(mType, t.cString());
    return RT_OK;
  }


  rtError fillColor(uint32_t& /*c*/) const
  {
    // TODO
    rtLogWarn("fillColor not implemented");
    return RT_OK;
  }

  rtError setFillColor(uint32_t c)
  {
    mFillColor[0] = (float)((c>>24)&0xff)/255.0f;
    mFillColor[1] = (float)((c>>16)&0xff)/255.0f;
    mFillColor[2] = (float)((c>>8)&0xff)/255.0f;
    mFillColor[3] = (float)((c>>0)&0xff)/255.0f;
    return RT_OK;
  }

  rtError lineColor(uint32_t& /*c*/) const
  {
    // TODO
    rtLogWarn("lineColor not implemented");
    return RT_OK;
  }

  rtError setLineColor(uint32_t c)
  {
    mLineColor[0] = (float)((c>>24)&0xff)/255.0f;
    mLineColor[1] = (float)((c>>16)&0xff)/255.0f;
    mLineColor[2] = (float)((c>>8)&0xff)/255.0f;
    mLineColor[3] = (float)((c>>0)&0xff)/255.0f;
    return RT_OK;
  }

  rtError lineWidth(float& w) const
  {
    w = mLineWidth;
    return RT_OK;
  }

  rtError setLineWidth(float w)
  {
    mLineWidth = w;
    return RT_OK;
  }

  // c is assumed to not be premultiplied
  void setFillColor(float* c)
  {
    mFillColor[0] = c[0];
    mFillColor[1] = c[1];
    mFillColor[2] = c[2];
    mFillColor[3] = c[3];
  }

  // c is assumed to not be premultiplied
  void setLineColor(float* c)
  {
    mLineColor[0] = c[0];
    mLineColor[1] = c[1];
    mLineColor[2] = c[2];
    mLineColor[3] = c[3];
  }



  virtual void draw();
  void drawLines();
  void drawPolygon();

private:
  float mFillColor[4];
  float mLineColor[4];
  float mLineWidth;
  char  mType[32];
  float mVerts[4096];  // max support 2048 points
  uint32_t mVertNumber;
};

#endif