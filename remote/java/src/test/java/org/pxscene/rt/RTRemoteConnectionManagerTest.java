package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.net.URI;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteConnectionManager;


/**
 * The junit examples connection manager tests
 */
public class RTRemoteConnectionManagerTest {


  /**
   * test get object by uri
   */
  @Test
  void testRTRemoteConnectionManager() {

    try {
      RTRemoteConnectionManager.getObjectProxy(URI.create("udp://127.0.0.1:3000/obj"));
    } catch (RTException e) {
      assertEquals(e.getMessage(), "unsupported scheme:udp");
    }

    try {
      RTRemoteConnectionManager.getObjectProxy(URI.create("tcp://127.0.0.1:12456/obj"));
    } catch (RTException e) {
      assertEquals(e.getMessage(), "failed to open socket");
    }

    try {
      RTRemoteConnectionManager.getObjectProxy(URI.create("tcp://127.0.0.1:546897/obj"));
    } catch (Exception e) {
      assertEquals(e.getMessage(), "port out of range:546897");
    }

  }
}
