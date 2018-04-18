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
   * service config
   */
  static public String remoteServerAddress = "224.10.10.12";
  static public Integer remoteServerPort = 10004;
  static public String innerServerAddress = "127.0.0.1";

  /**
   * crate server and client RTRemoteMulticastResolver
   *
   * @return the RTRemoteMulticastResolver
   */
  static RTRemoteMulticastResolver createServerAndReturnClientResolver()
      throws IOException, RTException {
    RTEnvironment.init();
    RTRemoteServer rtRemoteServer = new RTRemoteServer(InetAddress.getByName(remoteServerAddress), remoteServerPort,
        InetAddress.getByName(innerServerAddress));
    rtRemoteServer.registerObject("host_object", new SampleObject());
    return new RTRemoteMulticastResolver(
        InetAddress.getByName(remoteServerAddress), remoteServerPort);
  }
}
