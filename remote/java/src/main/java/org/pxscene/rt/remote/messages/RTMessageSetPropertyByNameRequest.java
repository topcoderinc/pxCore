package org.pxscene.rt.remote.messages;


import org.pxscene.rt.remote.RTRemoteMessageType;

/**
 * the set property by name request entity.
 */

public class RTMessageSetPropertyByNameRequest extends RTMessageSetPropertyRequest {

  /**
   * the remote object property name
   */
  protected String propertyName;

  public String getPropertyName() {
    return propertyName;
  }

  public void setPropertyName(String propertyName) {
    this.propertyName = propertyName;
  }

  /**
   * the entity constructor with type.
   */
  public RTMessageSetPropertyByNameRequest() {
    super(RTRemoteMessageType.SET_PROPERTY_BYNAME_REQUEST);
  }
}
