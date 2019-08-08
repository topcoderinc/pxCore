

#ifndef PXSCENE_MSEBASETRACKLIST_H
#define PXSCENE_MSEBASETRACKLIST_H

#include "MSEBaseObject.h"
#include "MSEBaseTrack.h"

class SourceBuffer;

/**
 * mse base track class, contains common properties
 */
class MSEBaseTrackList : public MSEBaseObject {
public:

  MSEBaseTrackList();

  rtDeclareObject(MSEBaseTrackList, MSEBaseObject);

  //readonly attribute unsigned long length;
  rtReadOnlyProperty(length, length, int);
  rtError length(int &c) const;

  //getter VideoTrack item(unsigned long index);
  rtError Get(uint32_t i, rtValue* value) const override;
  rtError Set(uint32_t i, const rtValue* value) override;
  
  //VideoTrack getTrackById(DOMString id);
  rtMethod1ArgAndReturn("getTrackById", getTrackById, rtString, rtObjectRef);
  rtError getTrackById(const rtString &id, rtObjectRef &out) const;

  //attribute EventHandler onchange;
  void onChange();
  //attribute EventHandler onaddtrack;
  void onAddTrack();
  //attribute EventHandler onremovetrack;
  void onRemoveTrack();

protected:
  std::vector<MSEBaseTrack*> mTracks;
};

class MSEAudioTrackList: public MSEBaseTrackList {
public:
  rtDeclareObject(MSEAudioTrackList, MSEBaseTrackList);
};

class MSEVideoTrackList: public MSEBaseTrackList {
public:
  rtDeclareObject(MSEVideoTrackList, MSEBaseTrackList);

  MSEVideoTrackList();

  rtReadOnlyProperty(selectedIndex, getSelectedIndex, int);
  rtError getSelectedIndex(int &v) const;


private:
  int mSelectedIndex;
};

class MSETextTrackList: public MSEBaseTrackList {
public:
  rtDeclareObject(MSETextTrackList, MSEBaseTrackList);
};


#endif

