package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;

import examples.TypeTest;
import java.io.IOException;
import java.net.URI;
import java.util.concurrent.ExecutionException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteConnectionManager;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;

/**
 * The junit examples of RTType.
 */
public class RTTypeTest {

  private RTRemoteMulticastResolver remoteMulticastResolver;
  private RTObject rtObject;

  /**
   * create server and resolver,rtObject for each junit exampels
   */
  @BeforeEach
  void init() throws IOException, RTException {
    remoteMulticastResolver = RTTestHelper.createServerAndReturnClientResolver();
    String objectName = "host_object";
    URI uri = remoteMulticastResolver.locateObject(objectName);
    rtObject = RTRemoteConnectionManager.getObjectProxy(uri);
  }

  /**
   * test types, the unit test base on src/main/examples/TypeTest, no exceptions mean test passed
   */
  @Test
  void testTypes() throws ExecutionException, InterruptedException, RTException {
    TypeTest rtRemoteTestClient = new TypeTest();
    rtRemoteTestClient.setTotalExamplesCount(0);
    rtRemoteTestClient.setSucceedExamplesCount(0);
    rtRemoteTestClient.doFunctionTest(rtObject, "onTick");
    rtRemoteTestClient.doObjectTest(rtObject, "objvar");
    rtRemoteTestClient.testAllTypes(rtObject);
  }


  /**
   * test PROPERTY_NOT_FOUND exception
   */
  @Test
  void testExceptions() throws RTException, ExecutionException, InterruptedException {
    try {
      rtObject.set("unknown_property", new RTValue(789892349L, RTValueType.VOIDPTR));
    } catch (Exception exception) {
      assertEquals(exception.getMessage(), "org.pxscene.rt.RTException: PROPERTY_NOT_FOUND");
    }
  }


}
