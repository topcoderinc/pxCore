package org.pxscene.rt.remote.messages;


import org.pxscene.rt.RTHelper;
import org.pxscene.rt.remote.RTRemoteMessage;
import org.pxscene.rt.remote.RTRemoteMessageType;

public class RTMessageGetPropertyRequest extends RTRemoteMessage {


  /**
   * the remote object id
   */
  protected String objectId;

  /**
   * the entity constructor with type
   */
  public RTMessageGetPropertyRequest(RTRemoteMessageType type) {
    super(type);
  }


  public String getObjectId() {
    return this.objectId;
  }

  public void setObjectId(String objectId) {
    RTHelper.ensureNotNull(objectId, "objectId");
    this.objectId = objectId;
  }
}
