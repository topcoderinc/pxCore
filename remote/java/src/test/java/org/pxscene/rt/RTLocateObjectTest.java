package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;

import java.io.IOException;
import java.net.URI;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteConnectionManager;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;

/**
 * The junit examples of locate obj
 */
public class RTLocateObjectTest {

  private RTRemoteMulticastResolver remoteMulticastResolver;

  /**
   * create server and client resovler
   */
  @BeforeEach
  void init() throws IOException, RTException {
    remoteMulticastResolver = RTTestHelper.createServerAndReturnClientResolver();
  }

  /**
   * test locate rt object
   */
  @Test
  void testLocateObj() throws IOException, RTException {
    String objectName = "host_object";
    URI uri = remoteMulticastResolver.locateObject(objectName);
    assertEquals(objectName, uri.getPath().substring(1));
    RTObject object = RTRemoteConnectionManager.getObjectProxy(uri);
    assertEquals(object.getId(), objectName);
  }


}
