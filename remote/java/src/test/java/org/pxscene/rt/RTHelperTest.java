package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

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
    TestObj testObj = new TestObj();
    testObj.field1 = "f1";
    testObj.setField2("f2");
    request.setObjectId("test");
    request.setPropertyName("field1");

    request.setCorrelationKey("testKey");
    assertEquals(RTHelper.getPropertyByName(testObj, request).getValue().getValue(), "f1");
  }

  /**
   * test obj get property by name with invalid type
   */
  @Test
  void testGetPropertyByNameWithInvalidType() {
    RTMessageGetPropertyByNameRequest request = new RTMessageGetPropertyByNameRequest();
    TestObj testObj = new TestObj();
    testObj.field1 = "f1";
    testObj.setField2("f2");
    request.setObjectId("test");
    request.setPropertyName("field1");
    try {
      request.setPropertyName("unknown_prop");
      assertEquals(RTHelper.getPropertyByName(testObj, request).getStatus().getCode(),
          RTStatusCode.PROP_NOT_FOUND);
    } catch (Exception e) {
      // TODO this should not be raise Exception , should be return PROP_NOT_FOUND
      fail("this should not be raise Exception , should be return PROP_NOT_FOUND");
    }
  }

  /**
   * test set object property by name
   */
  @Test
  void testSetPropertyName() {
    TestObj testObj = new TestObj();
    testObj.field1 = "f1";
    testObj.setField2("f2");

    RTMessageSetPropertyByNameRequest request = new RTMessageSetPropertyByNameRequest();
    request.setObjectId("test");
    request.setCorrelationKey("key");
    request.setPropertyName("field2");
    request.setValue(new RTValue("f1"));
    assertEquals(RTHelper.setPropertyByName(testObj, request).getStatusCode(), RTStatusCode.OK);

    request.setPropertyName("unknown_prop");
    assertEquals(RTHelper.setPropertyByName(testObj, request).getStatusCode(),
        RTStatusCode.PROPERTY_NOT_FOUND);
  }

  /**
   * test set object property by name with different value type
   */
  @Test
  void testSetPropertyNameWithDifferentType() {
    TestObj testObj = new TestObj();
    testObj.field1 = "f1";
    testObj.setField2("f2");

    RTMessageSetPropertyByNameRequest request = new RTMessageSetPropertyByNameRequest();
    request.setObjectId("test");
    request.setCorrelationKey("key");
    request.setPropertyName("field2");
    request.setValue(new RTValue("f1"));
    request.setPropertyName("field1");
    request.setValue(new RTValue("f1"));
    assertEquals(RTHelper.setPropertyByName(testObj, request).getStatusCode(),
        RTStatusCode.OK,
        "this should return ok, but is return  TYPE_MISMATCH");  // TODO this should return ok, but is return  TYPE_MISMATCH
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
    assertEquals(RTEnvironment.getRtFunctionMap().get("test").getFunctionName(), "test",
        "can update listener for RT function");
  }

  /**
   * test rt helper dump object
   */
  @Test
  void testDumpObject() {
    RTHelper.dumpObject("Test", new TestObj());
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
