package org.pxscene.rt.remote.messages;


import org.pxscene.rt.remote.RTRemoteMessageType;

public class RTMessageGetPropertyByNameRequest extends RTMessageGetPropertyRequest {

  /**
   * the set property name
   */

  protected String propertyName;

  public String getPropertyName() {
    return propertyName;
  }

  public void setPropertyName(String propertyName) {
    this.propertyName = propertyName;
  }

  /**
   * the entity constructor
   */
  public RTMessageGetPropertyByNameRequest() {
    super(RTRemoteMessageType.GET_PROPERTY_BYNAME_REQUEST);
  }
}
