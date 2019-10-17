#include "px_gst_pipeline.h"

namespace pxcore {

pxGstPipeline::pxGstPipeline(): m_pipeline(NULL), m_bus(NULL)
{
}

void pxGstPipeline::Configure(const char *name)
{
    m_pipeline = gst_pipeline_new(name);
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
}

void pxGstPipeline::Configure(GstElement *pipeline)
{
    m_pipeline = pipeline;
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
}

void pxGstPipeline::Destroy()
{
    if (m_pipeline) {
        gst_object_unref(m_pipeline);
        m_pipeline = NULL;
    }
    if (m_bus) {
        gst_object_unref(m_bus);
        m_bus = NULL;
    }
}

guint pxGstPipeline::setBusOnMessageCb(GstBusFunc cb, gpointer data)
{
    return gst_bus_add_watch(m_bus, (GstBusFunc)cb, data);
}

void pxGstPipeline::setBusSyncHandlerCb(GstBusSyncHandler cb, gpointer data)
{
    gst_bus_set_sync_handler(m_bus, (GstBusSyncHandler)cb, data, NULL);
}

}

