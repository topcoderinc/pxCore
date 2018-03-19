package org.pxscene.rt;


import examples.MethodTest;
import java.io.IOException;
import java.net.URI;
import java.rmi.server.ExportException;
import java.util.concurrent.ExecutionException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTConst;
import org.pxscene.rt.remote.RTRemoteConnectionManager;
import org.pxscene.rt.remote.RTRemoteMulticastResolver;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

/**
 * The junit examples of rt methods
 * this main example base on src/main/java/examples
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
   * check float equals
   *
   * @param v1 the value 1
   * @param v2 the value 2
   */
  private boolean checkEqualsFloat(Object v1, Object v2) {
    float eps = 0.001f;
    if (Math.abs((float) v1 - (float) v2) <= eps) {
      return true;
    }
    return false;
  }

//  /**
//   * do method unit tests, no exceptions mean test passed
//   */
//  @Test
//  void testMethods() throws ExecutionException, InterruptedException, RTException {
//    MethodTest methodTest = new MethodTest();
//    MethodTest.doMethodTest(methodTest, rtObject);
//  }

  /**
   * test method Method0AndReturn1
   */
  @Test
  void testMethod0AndReturn1() throws RTException, ExecutionException, InterruptedException {
    RTValue value = rtObject.sendReturns("method0AndReturn10").get();
    assertEquals(10, value.getValue());
    assertEquals(RTValueType.INT32, value.getType());
  }

  /**
   * test method twoFloatNumberSum
   */
  @Test
  void testMethodTwoFloatNumberSum() throws RTException, ExecutionException, InterruptedException {
    RTValue a = new RTValue(123.3f, RTValueType.FLOAT);
    RTValue b = new RTValue(12.3f, RTValueType.FLOAT);
    RTValue value = rtObject.sendReturns("twoFloatNumberSum", a, b).get();
    assertEquals(checkEqualsFloat(123.3f + 12.3f, value.getValue()), true);
    assertEquals(RTValueType.FLOAT, value.getType());
  }

  /**
   * test method twoIntNumberSum
   */
  @Test
  void testMethodTwoIntNumberSum() throws RTException, ExecutionException, InterruptedException {
    RTValue a = new RTValue(123, RTValueType.INT32);
    RTValue b = new RTValue(12, RTValueType.INT32);
    RTValue value = rtObject.sendReturns("twoIntNumberSum", a, b).get();
    assertEquals(123 + 12, value.getValue());
    assertEquals(RTValueType.INT32, value.getType());
  }

  /**
   * test method 1IntAndNoReturn
   */
  @Test
  void testMethod1IntAndNoReturn() throws RTException, ExecutionException, InterruptedException {
    RTValue a = new RTValue(11, RTValueType.INT32);
    rtObject.send("method1IntAndNoReturn", a).get();
    assertEquals(true, true, "Must have not return value");
  }

  /**
   * test unknown method
   */
  @Test
  void testUnknownMethod() throws RTException, ExecutionException, InterruptedException {
    RTValue a = new RTValue(11, RTValueType.INT32);
    try {
      RTValue value = rtObject.sendReturns("unknownMethod", a).get();
      assertEquals(value.getValue(), false);
    } catch (Exception e) {
      // TODO this should not be raise Exception , should be return METHOD_NOT_FOUND
      fail("this should not be raise Exception , should be return METHOD_NOT_FOUND");
    }
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
