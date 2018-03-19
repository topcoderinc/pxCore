package org.pxscene.rt;

import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteClientConnection;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * The junit examples remote client connection tests
 */
public class RTRemoteClientConnectionTest {

  /**
   * service config
   */
  static public String remoteServerAddress = "127.0.0.1";
  static public Integer remoteServerPort = 10004;

  /**
   * test get remote client connection object by hort and port
   */
  @Test
  void testRTRemoteClientConnection() throws RTException {
    try {
      RTRemoteClientConnection clientConnection = RTRemoteClientConnection.createTCPClientConnection(remoteServerAddress, remoteServerPort);
    } catch (RTException e) {
      assertEquals(e.getMessage(), "failed to open socket");
    }
  }

  /**
   * test get remote client connection object by invalid hort and port
   */
  @Test
  void testRTRemoteClientConnectionTimeout() throws  RTException {
    try {
      RTRemoteClientConnection clientConnection = RTRemoteClientConnection.createTCPClientConnection("10.211.55.10", remoteServerPort);
      assertEquals(true, clientConnection != null);
    } catch (RTException e) {
      assertEquals(e.getMessage(), "failed to open socket");
    }
  }
}
