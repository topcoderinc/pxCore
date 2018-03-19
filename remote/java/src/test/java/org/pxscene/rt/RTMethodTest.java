package org.pxscene.rt;


import examples.MethodTest;
import java.io.IOException;
import java.net.URI;
import java.util.concurrent.ExecutionException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTConst;
import org.pxscene.rt.remote.RTRemoteConnectionManager;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;

/**
 * The junit examples of rt methods
 * this main exmple base on src/main/java/examples
 */
public class RTMethodTest {

  private RTRemoteMulticastResolver remoteMulticastResolver;
  private RTObject rtObject;

  /**
   * create server and remote multicast resolver for each junit example
   *
   * @throws IOException the socket exceptions
   * @throws RTException the logic exceptions
   */
  @BeforeEach
  void init() throws IOException, RTException {
    remoteMulticastResolver = RTTestHelper.createServerAndReturnClientResolver();
    String objectName = "host_object";
    URI uri = remoteMulticastResolver.locateObject(objectName);
    rtObject = RTRemoteConnectionManager.getObjectProxy(uri);
  }

  /**
   * do method unit tests, no exceptions mean test passed
   */
  @Test
  void testMethods() throws ExecutionException, InterruptedException, RTException {
    MethodTest methodTest = new MethodTest();
    MethodTest.doMethodTest(methodTest, rtObject);
  }

  /**
   * set environment as client mode, then test method, no exceptions mean test passed
   */
  @Test
  void testAsClientMode() throws RTException, ExecutionException, InterruptedException {
    // test method that passed RtFunction and invoke this function , rtMethod2ArgAndNoReturn
    RTEnvironment.setRunMode(RTConst.CLIENT_MODE);
    RTFunction function = new RTFunction(rtValueList -> {

    });

    RTEnvironment.setRunMode(RTConst.CLIENT_MODE);
    rtObject.send("method2FunctionAndNoReturn", new RTValue(function),
        new RTValue(10, RTValueType.INT32)).get();
  }
}
