package org.pxscene.rt;

import examples.server.SampleObject;
import java.io.IOException;
import java.net.InetAddress;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;
import org.pxscene.rt.remote.RTRemoteServer;


/**
 * The helper class of junit tests.
 */
public class RTTestHelper {

  /**
   * crate server and client RTRemoteMulticastResolver
   *
   * @return the RTRemoteMulticastResolver
   */
  static RTRemoteMulticastResolver createServerAndReturnClientResolver()
      throws IOException, RTException {
    RTEnvironment.init();
    RTRemoteServer rtRemoteServer = new RTRemoteServer(InetAddress.getByName("224.10.10.12"), 10004,
        InetAddress.getByName("127.0.0.1"));
    rtRemoteServer.registerObject("host_object", new SampleObject());
    return new RTRemoteMulticastResolver(
        InetAddress.getByName("224.10.10.12"), 10004);
  }
}
