#include "GStreamPlayer.h"
#include "helper.h"

const gint64 TIME_UNIT = 1000 * 1000 * 1000;

static gboolean GStreamer_BusMessage(GstBus * /* bus */, GstMessage *msg, GStreamPlayer *app)
{
  GError *err;
  gchar *debug_info;


  if (gst_is_video_overlay_prepare_window_handle_message(msg)) {
    guintptr handler = (guintptr) app;
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (GST_MESSAGE_SRC(msg)), contentView(handler));
  }

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &debug_info);
      g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error(&err);
      g_free(debug_info);
      break;
    case GST_MESSAGE_EOS:
      if (app->isPlaying()) {
        app->emit("ended");
        app->emit("stop");
      }

      app->setPlaying(false);
      app->setEnded(true);
      if (app->isLoop()) {
        g_warning("loop param is true, so play again!!");
        app->play(); // play again if loop is true
      }
      break;
    case GST_MESSAGE_DURATION:
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
      if (new_state == GST_STATE_PAUSED) {
        if (app->isPlaying()) {
          app->emit("pause");
        }
        app->setPlaying(false);
      } else if (new_state == GST_STATE_PLAYING) {
        if (!app->isPlaying()) {
          app->emit("play");
          app->emit("playing");
        }
        app->setPlaying(true);
      }
      break;
    }
    default:;
  }
  return TRUE;
}


/* This method is called by the need-data signal callback, we feed data into the
 * appsrc with the requested size.
 */
static void GStreamer_FeedData(GstElement */* appsrc*/, guint size, GStreamPlayer *app)
{
  // the file is loaded
  if (!app->isLoaded() && app->getDuration() > 0) {
    app->emit("canplay");  // data loaded, now it can play
    app->emit("loadeddata"); // data loaded
    app->emit("durationchange"); // duration parsed, from 0 to real duration
    app->emit("canplaythrough"); // this is local file, no need buffer
    app->setLoaded(true);
    g_warning("loaded first frame !! %s", app->isAutoPlay() ? "true" : "false");
    if (!app->isAutoPlay()) {
      app->pause();
    }
    app->setReadyState(4);
  } else {
    app->setReadyState(3);
  }
  GstBuffer *buffer;
  GstFlowReturn ret;
  if (app->getOffset() >= app->getLength()) {
    /* we are EOS, send end-of-stream */
    g_signal_emit_by_name(app->getAppsrc(), "end-of-stream", &ret);
    return;
  }

  /* read the amount of data, we are allowed to return less if we are EOS */
  buffer = gst_buffer_new();

  if (app->getOffset() + size > app->getLength())
    size = app->getLength() - app->getOffset();

  gst_buffer_append_memory(buffer,
                           gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                                  app->getData(), app->getLength(), app->getOffset(), size, NULL,
                                                  NULL));

  /* we need to set an offset for random access */
  GST_BUFFER_OFFSET (buffer) = app->getOffset();
  GST_BUFFER_OFFSET_END (buffer) = app->getOffset() + size;

  g_signal_emit_by_name(app->getAppsrc(), "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  app->setOffset(app->getOffset() + size);
}

/* called when appsrc wants us to return data from a new position with the next
 * call to push-buffer. */
static gboolean GStreamer_SeekData(GstElement */*appsrc */, guint64 position, GStreamPlayer *app)
{
  app->setOffset(position);
  return TRUE;
}

/* this callback is called when playbin has constructed a source object to read
 * from. Since we provided the appsrc:// uri to playbin, this will be the
 * appsrc that we must handle. We set up some signals to push data into appsrc
 * and one to perform a seek. */
static void GStreamer_FoundSource(GObject */*object */, GObject *orig, GParamSpec *pspec, GStreamPlayer *app)
{
  /* get a handle to the appsrc */
  GstElement *appsrc;
  g_object_get(orig, pspec->name, &appsrc, NULL);


  app->setAppsrc(appsrc);

  /* we can set the length in appsrc. This allows some elements to estimate the
   * total duration of the stream. It's a good idea to set the property when you
   * can but it's not required. */
  g_object_set(app->getAppsrc(), "size", (gint64) app->getLength(), NULL);
  gst_util_set_object_arg(G_OBJECT (app->getAppsrc()), "stream-type", "random-access");

  /* configure the appsrc, we will push a buffer to appsrc when it needs more
   * data */
  g_signal_connect (app->getAppsrc(), "need-data", G_CALLBACK(GStreamer_FeedData), app);
  g_signal_connect (app->getAppsrc(), "seek-data", G_CALLBACK(GStreamer_SeekData), app);
}


static gboolean
GStreamer_DrawCallback(GstElement */*gl_sink */, void */*context */, GstSample *sample, GStreamPlayer *app)
{
  GstVideoFrame v_frame;
  GstVideoInfo v_info;
  GstBuffer *buf = gst_sample_get_buffer(sample);
  GstCaps *caps = gst_sample_get_caps(sample);
  gst_video_info_from_caps(&v_info, caps);

  if (gst_video_frame_map(&v_frame, &v_info, buf, (GstMapFlags) (GST_MAP_READ))) {
    int height = GST_VIDEO_FRAME_HEIGHT(&v_frame);
    int width = GST_VIDEO_FRAME_WIDTH(&v_frame);


    guint8 *pixels = static_cast<guint8 *>GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
    guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&v_frame, 0);
    guint pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (&v_frame, 0);
    if (app->getFrameCallback()) {
      app->getFrameCallback()(pixels, width, height, stride, pixel_stride);
    }
    gst_video_frame_unmap(&v_frame);
  }

  return TRUE;
}

GStreamPlayer::GStreamPlayer(const char *file, const FrameCallback &frameCallback) :
    playbin(nullptr), offset(0), playing(false), loaded(false), autoPlay(false), seeking(false),
    runtimeError(""), ended(false), loop(false), readyState(0), startDate(0)
{

  if (file) {
    loadFile(file);
  }
  GStreamPlayer::frameCallback = frameCallback;
}


void GStreamPlayer::release()
{
  emit("closed", nullptr);
  if (playbin) {
    gst_element_set_state(playbin, GST_STATE_NULL);
    /* free the file */
    g_mapped_file_unref(file);
    playbin = nullptr;
  }
  ended = true;
}

GStreamPlayer::~GStreamPlayer()
{
  release();
  releaseView((guintptr) this);
}

void GStreamPlayer::loadFile(const char *fileUri)
{
  emit("loadstart");
  release();

  ended = false;
  loaded = false;
  seeking = false;
  playing = false;
  readyState = 0;

  GError *error = NULL;

  /* try to open the file as an mmapped file */
  file = g_mapped_file_new(fileUri, FALSE, &error);
  if (error) {
    emit("abort");
    setRuntimeError("failed to open file");
    emit("error");
    g_warning("failed to open file: %s\n", error->message);
    g_error_free(error);
    release();
    return;
  }
  emit("loadedmetadata"); // file loaded
  readyState = 1;
  /* get some vitals, this will be used to read data from the mmapped file and
   * feed it to appsrc. */
  length = g_mapped_file_get_length(file);
  data = (guint8 *) g_mapped_file_get_contents(file);
  offset = 0;

  playbin = gst_element_factory_make("playbin", "spark-playbin");

  bus = gst_pipeline_get_bus(GST_PIPELINE (playbin));

  /* add watch for messages */
  gst_bus_add_watch(bus, (GstBusFunc) GStreamer_BusMessage, this);

  /* set to read from appsrc */
  g_object_set(playbin, "uri", "appsrc://", NULL);

  /* get notification when the source is created so that we get a handle to it
   * and can configure it */
  g_signal_connect (playbin, "deep-notify::source", (GCallback) GStreamer_FoundSource, this);
  readyState = 2;

  GstElement *glimagesink = gst_element_factory_make("glimagesink", "spark-video-glimage");
  g_object_set(playbin, "video-sink", glimagesink, NULL);
  g_signal_connect(G_OBJECT(glimagesink), "client-draw", G_CALLBACK(GStreamer_DrawCallback), this);
  gst_element_set_state(playbin, GST_STATE_PLAYING);
  startDate = getCurrentTime();
}

void GStreamPlayer::play()
{
  gst_element_set_state(playbin, GST_STATE_PLAYING);
  if (this->getDuration() > 0 && this->getPosition() >= 0 && this->getDuration() - this->getPosition() < 0.1) {
    // think finished, replay
    setPosition(0);
  }
}

void GStreamPlayer::pause()
{
  gst_element_set_state(playbin, GST_STATE_PAUSED);
}

float GStreamPlayer::getDuration() const
{
  gint64 duration = -1;
  if (!gst_element_query_duration(playbin, GST_FORMAT_TIME, &duration)) {
    return -1.0;
  }
  return duration / (TIME_UNIT * 1.0);
}

float GStreamPlayer::getPosition() const
{
  gint64 position = -1;
  if (!gst_element_query_position(playbin, GST_FORMAT_TIME, &position)) {
    return -1.0;
  }
  return position / (TIME_UNIT * 1.0);
}

void GStreamPlayer::setPosition(float position)
{
  auto seekTime = (gint64) (position * TIME_UNIT);
  auto d = (gint64) (getDuration() * TIME_UNIT);
  if (seekTime > d) {
    seekTime = d - TIME_UNIT;
  }
  seeking = true;
  this->emit("seeking");
  if (!gst_element_seek(playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                        GST_SEEK_TYPE_SET, seekTime,
                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
    g_warning("Seek failed!\n");
  }
  seeking = false;
  emit("seeked");
}

void GStreamPlayer::setFrameCallback(const FrameCallback &frameCallback)
{
  GStreamPlayer::frameCallback = frameCallback;
}

GstElement *GStreamPlayer::getPlaybin() const
{
  return playbin;
}

GstElement *GStreamPlayer::getAppsrc() const
{
  return appsrc;
}

GMappedFile *GStreamPlayer::getFile() const
{
  return file;
}

GstBus *GStreamPlayer::getBus() const
{
  return bus;
}

guint8 *GStreamPlayer::getData() const
{
  return data;
}

gsize GStreamPlayer::getLength() const
{
  return length;
}

guint64 GStreamPlayer::getOffset() const
{
  return offset;
}

void GStreamPlayer::setAppsrc(GstElement *appsrc)
{
  GStreamPlayer::appsrc = appsrc;
}

void GStreamPlayer::setOffset(guint64 offset)
{
  GStreamPlayer::offset = offset;
}

const FrameCallback &GStreamPlayer::getFrameCallback() const
{
  return frameCallback;
}

const EventCallback &GStreamPlayer::getEventCallback() const
{
  return eventCallback;
}

void GStreamPlayer::setEventCallback(const EventCallback &eventCallback)
{
  GStreamPlayer::eventCallback = eventCallback;
}

bool GStreamPlayer::isPlaying() const
{
  return playing;
}

void GStreamPlayer::setPlaying(bool playing)
{
  GStreamPlayer::playing = playing;
}

bool GStreamPlayer::isLoaded() const
{
  return loaded;
}

void GStreamPlayer::setLoaded(bool loaded)
{
  GStreamPlayer::loaded = loaded;
}

bool GStreamPlayer::isAutoPlay() const
{
  return autoPlay;
}

void GStreamPlayer::setAutoPlay(bool autoPlay)
{
  GStreamPlayer::autoPlay = autoPlay;
}


bool GStreamPlayer::isEnded() const
{
  return ended;
}

void GStreamPlayer::setEnded(bool ended)
{
  GStreamPlayer::ended = ended;
}

bool GStreamPlayer::isLoop() const
{
  return loop;
}

void GStreamPlayer::setLoop(bool loop)
{
  GStreamPlayer::loop = loop;
}

int GStreamPlayer::getReadyState() const
{
  return readyState;
}

void GStreamPlayer::setReadyState(int readyState)
{
  GStreamPlayer::readyState = readyState;
}

bool GStreamPlayer::isSeeking() const
{
  return seeking;
}

const std::string &GStreamPlayer::getRuntimeError() const
{
  return runtimeError;
}

void GStreamPlayer::setRuntimeError(const std::string &runtimeError)
{
  GStreamPlayer::runtimeError = runtimeError;
}
