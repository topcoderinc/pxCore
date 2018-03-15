package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTConst;
import org.pxscene.rt.remote.messages.RTMessageGetPropertyByNameRequest;
import org.pxscene.rt.remote.messages.RTMessageSetPropertyByNameRequest;

/**
 * The junit examples of test RTHelper.
 */
public class RTHelperTest {

  /**
   * test obj get property by name
   */
  @Test
  void testGetPropertyByName() {
    RTMessageGetPropertyByNameRequest request = new RTMessageGetPropertyByNameRequest();
    ObjTest objTest = new ObjTest();
    objTest.field1 = "f1";
    objTest.setField2("f2");
    request.setObjectId("test");
    request.setPropertyName("field1");

    request.setCorrelationKey("testKey");
    assertEquals(RTHelper.getPropertyByName(objTest, request).getValue().getValue(), "f1");

    try {
      request.setPropertyName("unkown_prop");
      assertEquals(RTHelper.getPropertyByName(objTest, request).getStatus().getCode(),
          RTStatusCode.PROP_NOT_FOUND);
    } catch (Exception e) {
      // TODO this should not be raise Exception , should be return PROP_NOT_FOUND
      assertEquals(false, true);
    }
  }

  /**
   * test set object property by name
   */
  @Test
  void testSetPropertyName() {
    ObjTest objTest = new ObjTest();
    objTest.field1 = "f1";
    objTest.setField2("f2");

    RTMessageSetPropertyByNameRequest request = new RTMessageSetPropertyByNameRequest();
    request.setObjectId("test");
    request.setCorrelationKey("key");
    request.setPropertyName("field2");
    request.setValue(new RTValue("f1"));
    assertEquals(RTHelper.setPropertyByName(objTest, request).getStatusCode(), RTStatusCode.OK);

    request.setPropertyName("field1");
    request.setValue(new RTValue("f1"));
    assertEquals(RTHelper.setPropertyByName(objTest, request).getStatusCode(),
        RTStatusCode.OK);  // TODO this should return ok, but is return  TYPE_MISMATCH

    request.setPropertyName("unkonw_prop");
    assertEquals(RTHelper.setPropertyByName(objTest, request).getStatusCode(),
        RTStatusCode.PROPERTY_NOT_FOUND);
  }

  /**
   * test rtHelper check null
   */
  @Test
  void testNull() {
    try {
      RTHelper.ensureNotNull(null, "testValue");
    } catch (Exception exception) {
      assertEquals(exception instanceof NullPointerException, true);
    }
  }

  /**
   * test rt helper update function listener
   *
   * @throws RTException the rtFunction exception
   */
  @Test
  void testUpdateListenerForRTFuction() throws RTException {
    RTFunction oldFUnction = new RTFunction();
    oldFUnction.setObjectId("global");
    oldFUnction.setFunctionName("test");
    RTFunction newFunction = RTHelper.updateListenerForRTFuction(null, oldFUnction);
    try {
      newFunction.getListener().invoke(null);
    } catch (Exception e) {
      assertEquals(e.getClass(), RTException.class); // protocol is null
    }
    assertEquals(RTEnvironment.getRtFunctionMap().get("test").getFunctionName(), "test");
  }

  /**
   * test rt helper dump object
   */
  @Test
  void testDumpObject() {
    RTHelper.dumpObject("Test", new ObjTest());
  }

  /**
   * test rt environment
   */
  @Test
  void testRTEnv() {
    RTEnvironment env = new RTEnvironment();
    env.setRunMode(RTConst.SERVER_MODE);
    assertEquals(RTConst.SERVER_MODE, RTEnvironment.getRunMode());

    RTConst rtConst = new RTConst();
    assertEquals(rtConst.SERVER_MODE, RTConst.SERVER_MODE);
  }


}
