#ifndef PXSCENE_PXAAMPPLAYER_H
#define PXSCENE_PXAAMPPLAYER_H


#include "pxObject.h"
#include "pxOffscreen.h"
#include "pxTexture.h"


#include "MediaSource.h"

/**
 * aamp player class, like <video>
 * most of properties are from here https://developer.mozilla.org/en-US/docs/Web/API/HTMLMediaElement
 */
class pxAAMPPlayer : public pxObject {
public:
  rtDeclareObject(pxAAMPPlayer, pxObject);

  rtProperty(src, getSrc, updateSrc, rtString);

  rtReadOnlyProperty(currentSrc, getCurrentSrc, rtString);

  rtReadOnlyProperty(currentTime, getCurrentTime, double);

  rtReadOnlyProperty(duration, getDuration, double);

  rtProperty(defaultMuted, getDefaultMuted, setDefaultMuted, bool);

  rtMethodNoArgAndNoReturn("pause", pause);

  rtMethodNoArgAndNoReturn("play", play);

  rtMethod1ArgAndNoReturn("fastSeek", fastSeek, double);

  rtReadOnlyProperty(mediaSource, getMediaSource, rtObjectRef);

  /**
   * create new aamp player
   * @param scene
   */
  pxAAMPPlayer(pxScene2d *scene);

  rtError getCurrentSrc(rtString &v) const
  {
    return getSrc(v);
  }

  /**
   * get src
   * @param v the src ref
   */
  rtError getSrc(rtString &v) const;

  /**
   * get current stream duration
   * @param v the output value
   * @return
   */
  rtError getDuration(double &v) const;


  /**
   * get current time
   * @param v the output value
   * @return
   */
  rtError getCurrentTime(double &v) const;

  /**
   * get is Muted
   * @param v the output value
   * @return
   */
  rtError getDefaultMuted(bool &v) const;

  /**
   * set Muted
   * @param v the input value
   * @return
   */
  rtError setDefaultMuted(bool const &v);

  /**
   * update url
   * @param newUrl the url
   */
  rtError updateSrc(rtString const &newSrc);

  /**
   * pause player
   */
  rtError pause();

  /**
   * play video
   */
  rtError play();

  /**
   * fast seek
   * @param time the dest time
   */
  rtError fastSeek(double time);


  /**
   * get media source
   * @param v the output value
   * @return
   */
  rtError getMediaSource(rtObjectRef &v) const;

  virtual ~pxAAMPPlayer();

  /**
   * spark init function
   */
  virtual void onInit();


  /**
   * when pxObject dispose
   * @param pumpJavascript from js invoke ?
   */
  virtual void dispose(bool pumpJavascript);

  /**
   * spark draw method
   */
  virtual void draw();

  /**
   * spark update function
   * @param t the current time
   * @param updateChildren is need updateChildren?
   */
  void update(double t, bool updateChildren) override;

protected:
  bool mMuted;
  rtString mSrc;
  pxOffscreen *mOffscreen;
  pxTextureRef mTextureRef;
  long mRenderFrameId;
  double mTimeTik;
  double mPreviousFrameTime;
  MediaSource *mMediaSource;
};


void prepareAAMP();

#endif //PXSCENE_PXAAMPPLAYER_H
