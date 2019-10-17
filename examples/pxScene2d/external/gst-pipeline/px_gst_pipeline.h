#ifndef PXGST_PIPELINE_H_
#define PXGST_PIPELINE_H_

#include <gst/gst.h>

namespace pxcore {

class pxGstPipeline
{
public:
    pxGstPipeline();

	void Configure(const char *name);
    void Configure(GstElement *pipeline);
    void Destroy();

	// void Send(MediaType mediaType, const void *ptr, size_t len) = 0;
    // void Stop(bool keepLastFrame) = 0;
	// void Pause(bool pause) = 0;

    guint setBusOnMessageCb(GstBusFunc cb, gpointer data);
    void setBusSyncHandlerCb(GstBusSyncHandler cb, gpointer data);

    GstElement *pipeline() { return m_pipeline; }
    GstBus     *bus() { return m_bus; }

private:
    GstElement *m_pipeline;
    GstBus     *m_bus;
};

}

#endif // PXGST_PIPELINE_H_
