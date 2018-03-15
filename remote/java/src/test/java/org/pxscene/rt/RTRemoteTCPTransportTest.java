package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.io.IOException;
import java.net.URI;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;
import org.pxscene.rt.remote.RTRemoteTCPTransport;


/**
 * The junit examples of RTRemoteTCPTransport.
 */
public class RTRemoteTCPTransportTest {


  /**
   * test create with unkown host
   */
  @Test
  void testRTRemoteTCPTransport() throws RTException {
    try {
      RTRemoteTCPTransport tcpTransport = new RTRemoteTCPTransport("unkown-zzsd", 123);
    } catch (RTException e) {
      assertEquals(e.getMessage(), "failed to lookup unkown-zzsd");
    }
  }

  /**
   * test create transport and close to test exceptions
   */
  @Test
  void test() throws IOException, RTException {
    RTRemoteMulticastResolver resolver = RTTestHelper.createServerAndReturnClientResolver();
    URI uri = resolver.locateObject("host_object");

    String host = uri.getHost();
    int port = uri.getPort();
    RTRemoteTCPTransport tcpTransport = new RTRemoteTCPTransport(host, port);
    tcpTransport.open();

    tcpTransport.close();

    try {
      tcpTransport.close();
    } catch (Exception e) {
      assertEquals(e.getMessage(), "failed to close socket");
    }

    try {
      tcpTransport.send(new byte[]{1, 2, 3, 4});
    } catch (Exception e) {
      assertEquals(e.getMessage(), "error sending message");
    }
  }
}
