#ifndef PXSCENE_PXAAMPPLAYER_H
#define PXSCENE_PXAAMPPLAYER_H


#include "pxObject.h"
#include "AAMPView.h"

/**
 * aamp player class
 */
class pxAAMPPlayer : public pxObject {
public:
  rtDeclareObject(pxAAMPPlayer, pxObject);

  rtProperty(url, getUrl, updateUrl, rtString);

  rtMethodNoArgAndNoReturn("pause", pause);

  rtMethodNoArgAndNoReturn("play", play);

  /**
   * create new aamp player
   * @param scene
   */
  pxAAMPPlayer(pxScene2d *scene) : pxObject(scene)
  {
    rtLogSetLevel(RT_LOG_INFO);
    rtLogInfo("create new pxAAMPPlayer...");
    initAAMP();
  }

  rtError getUrl(rtString &url) const
  {
    url = rtString(mUrl.c_str());
    return RT_OK;
  };

  /**
   * pause player
   * @return
   */
  rtError pause();

  /**
   * play video
   * @return
   */
  rtError play();

  /**
   * update url
   * @param newUrl the url
   * @return
   */
  rtError updateUrl(rtString const &newUrl);

  void initAAMP();

  virtual ~pxAAMPPlayer();

  virtual void onInit();


  /**
   * when pxObject dispose
   * @param pumpJavascript from js invoke ?
   */
  virtual void dispose(bool pumpJavascript);

  void getGlobalPosition(float *p);


  /**
   * update aamp view size
   */
  void updateAAMPSize();

private:

  std::string mUrl;

  //AAMPView *mAAMPView;

  float diffX = 10;
  float diffY = 60;
};


#endif //PXSCENE_PXAAMPPLAYER_H
