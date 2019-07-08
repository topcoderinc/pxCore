

#ifndef GST_0100_GSTREAMPLAYER_H
#define GST_0100_GSTREAMPLAYER_H

#include <gst/gst.h>
#include <functional>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include <event.h>


typedef std::function<void(guint8 *buffer, int w, int h, guint stride, guint pixel_stride)> FrameCallback;
typedef std::function<void(const char *event, void *data)> EventCallback;

class GStreamPlayer {

public:

  GStreamPlayer(const char *file, const FrameCallback &frameCallback);

  ~GStreamPlayer();

  float getDuration() const;

  float getPosition() const;

  void setPosition(float position);

  void play();

  void pause();

  void setFrameCallback(const FrameCallback &frameCallback);

  void loadFile(const char *file);

  GstElement *getPlaybin() const;

  GstElement *getAppsrc() const;

  GMappedFile *getFile() const;

  GstBus *getBus() const;

  guint8 *getData() const;

  gsize getLength() const;

  guint64 getOffset() const;

  void setAppsrc(GstElement *appsrc);

  void setOffset(guint64 offset);

  const FrameCallback &getFrameCallback() const;

  const EventCallback &getEventCallback() const;

  void setEventCallback(const EventCallback &eventCallback);

  bool isPlaying() const;

  void setPlaying(bool playing);

  void release();

  bool isLoaded() const;

  void setLoaded(bool loaded);

  bool isAutoPlay() const;

  void setAutoPlay(bool autoPlay);

  void emit(const char *event, void *data = nullptr)
  {
    if (eventCallback) {
      eventCallback(event, data);
    } else {
      g_warning("GStreamer Player event emit callback should not null!!");
    }
  }

  bool seekable()
  {
    return playbin && file;
  }


  bool isEnded() const;

  void setEnded(bool ended);

  bool isLoop() const;

  void setLoop(bool loop);

  int getReadyState() const;

  void setReadyState(int readyState);

  bool isSeeking() const;

  const std::string &getRuntimeError() const;

  void setRuntimeError(const std::string &runtimeError);

  long getCurrentTime()
  {
    struct timeval tp{};
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
  }

private:
  GstElement *playbin;
  GstElement *appsrc;
  GMappedFile *file;
  GstBus *bus;
  guint8 *data;
  gsize length;
  guint64 offset;

  FrameCallback frameCallback;
  EventCallback eventCallback;
  bool playing;
  bool loaded;
  bool autoPlay;
  bool seeking;
  std::string runtimeError;
  bool ended;
  bool loop;
  int readyState;
  long startDate;
};


#endif
