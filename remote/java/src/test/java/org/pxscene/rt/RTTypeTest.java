package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;

import examples.TestObject;
import java.io.IOException;
import java.math.BigInteger;
import java.net.URI;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
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
   * test function type ,rtValue is a new value, but rtFunction should be has the same ref/hashCode
   */
  @Test
  void testFunctionType() throws ExecutionException, InterruptedException, RTException {
    RTValue oldRtValue = new RTValue(new RTFunction(rtValueList -> {
    }));
    rtObject.set("onTick", oldRtValue).get();
    Future<RTValue> valueFuture = rtObject.get("onTick");
    RTValue rtValue = valueFuture.get();
    ((RTFunction) rtValue.getValue()).getListener().invoke(null);
    assertEquals(true, checkEqualsFunction(oldRtValue, rtValue));
    assertEquals(rtValue.getValue(), oldRtValue.getValue());
    assertEquals(rtValue.getValue().hashCode(), oldRtValue.getValue().hashCode());
  }

  /**
   * test object type, the object should be has the same ref/hasCode
   */
  @Test
  void testObjectType() throws ExecutionException, InterruptedException, RTException {
    TestObject oldObj = new TestObject("type test");
    RTValue oldRtValue = new RTValue(oldObj, RTValueType.OBJECT);
    rtObject.set("objvar", oldRtValue).get();
    RTValue newObj = rtObject.get("objvar").get();
    TestObject testObject = (TestObject) newObj.getValue();
    assertEquals(oldObj.getId(), testObject.getId(), "Object type must have the same id");
    assertEquals(oldObj, testObject);
    assertEquals(oldObj.hashCode(), testObject.hashCode());
  }

  /**
   * test INT8 Type
   */
  @Test
  void testInt8Type() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("int8", new RTValue((byte) -128, RTValueType.INT8)).get();
    Object newVal = rtObject.get("int8").get().getValue();
    assertEquals((byte) -128, newVal);

    rtObject.set("int8", new RTValue((byte) -127, RTValueType.INT8)).get();
    Object newVal2 = rtObject.get("int8").get().getValue();
    assertEquals((byte) -127, newVal2);
  }

  /**
   * test UINT8 type
   */
  @Test
  void testUint8() throws RTException, ExecutionException, InterruptedException {
    rtObject.set("uint8", new RTValue((short)255, RTValueType.UINT8)).get();
    Object newVal = rtObject.get("uint8").get().getValue();
    assertEquals((short) 255, newVal);
  }

  /**
   * test INT32 Type with negative
   */
  @Test
  void testInt32TypeNegative() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("int32", new RTValue(-2147483648, RTValueType.INT32)).get();
    Object newVal = rtObject.get("int32").get().getValue();
    assertEquals(-2147483648, newVal);
  }

  /**
   * test INT32 Type with positive
   */
  @Test
  void testInt32TypePositive() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("int32", new RTValue(214748364, RTValueType.INT32)).get();
    Object newVal = rtObject.get("int32").get().getValue();
    assertEquals(214748364, newVal);
  }

  /**
   * test UINT32 Type
   */
  @Test
  void testUInt32Type() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("uint32", new RTValue(4294967295L, RTValueType.UINT32)).get();
    Object newVal = rtObject.get("uint32").get().getValue();
    assertEquals(4294967295L, newVal);
  }


  /**
   * test INT64 Type with positive
   */
  @Test
  void testInt64TypePositive() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("int64", new RTValue(9223372036854775807L, RTValueType.INT64)).get();
    Object newVal = rtObject.get("int64").get().getValue();
    assertEquals(9223372036854775807L, newVal);
  }

  /**
   * test INT64 Type with negative
   */
  @Test
  void testInt64TypeNegative() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("int64", new RTValue(-9223372036854775808L, RTValueType.INT64)).get();
    Object newVal = rtObject.get("int64").get().getValue();
    assertEquals(-9223372036854775808L, newVal);
  }

  /**
   * test UINT64 Type
   */
  @Test
  void testUInt64Type() throws ExecutionException, InterruptedException, RTException {
    BigInteger bigInteger = new BigInteger("18446744073709551615");
    rtObject.set("uint64", new RTValue(bigInteger, RTValueType.UINT64)).get();
    Object newVal = rtObject.get("uint64").get().getValue();
    assertEquals(bigInteger, newVal);
  }


  /**
   * test FLOAT Type with positive
   */
  @Test
  void testFloatTypePositive() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("ffloat", new RTValue(12.0123123f, RTValueType.FLOAT)).get();
    Object newVal = rtObject.get("ffloat").get().getValue();
    assertEquals(true, checkEqualsFloat(12.0123123f, newVal));
  }

  /**
   * test DOUBLE Type with negative
   */
  @Test
  void testFloatTypeNegative() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("ffloat", new RTValue(-123.8818f, RTValueType.FLOAT)).get();
    Object newVal = rtObject.get("ffloat").get().getValue();
    assertEquals(true, checkEqualsFloat(-123.8818f, newVal));
  }

  /**
   * test DOUBLE Type with positive
   */
  @Test
  void testDoubleTypePositive() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("ddouble", new RTValue(1231.12312312312, RTValueType.DOUBLE)).get();
    Object newVal = rtObject.get("ddouble").get().getValue();
    assertEquals(true, checkEqualsDouble(1231.12312312312, newVal));
  }

  /**
   * test FLOAT Type with negative
   */
  @Test
  void testDoubleTypeNegative() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("ddouble", new RTValue(-1231.12312312312, RTValueType.DOUBLE)).get();
    Object newVal = rtObject.get("ddouble").get().getValue();
    assertEquals(true, checkEqualsDouble(-1231.12312312312, newVal));
  }

  /**
   * test BOOLEAN Type value true
   */
  @Test
  void testBooleanTypeTrue() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("bbool", new RTValue(true, RTValueType.BOOLEAN)).get();
    Object newVal = rtObject.get("bbool").get().getValue();
    assertEquals(true, newVal);
  }

  /**
   * test BOOLEAN Type value true
   */
  @Test
  void testBooleanTypeFalse() throws ExecutionException, InterruptedException, RTException {
    rtObject.set("bbool", new RTValue(false, RTValueType.BOOLEAN)).get();
    Object newVal = rtObject.get("bbool").get().getValue();
    assertEquals(false, newVal);
  }

  /**
   * test STRING Type with path like
   */
  @Test
  void testStringTypeWithPathLike() throws ExecutionException, InterruptedException, RTException {
    String tString = "implemented in both /Library/Java/JavaVirtualMachines/jdk1.8.0_40.jdk/Conten";
    rtObject.set("string", new RTValue(tString, RTValueType.STRING)).get();
    Object newVal = rtObject.get("string").get().getValue();
    assertEquals(tString, newVal);
  }

  /**
   * test STRING Type with json like
   */
  @Test
  void testStringTypeWithJsonLike() throws ExecutionException, InterruptedException, RTException {
    String tString = "{\"jsonKey\":\"values\"}";
    rtObject.set("string", new RTValue(tString, RTValueType.STRING)).get();
    Object newVal = rtObject.get("string").get().getValue();
    assertEquals(tString, newVal);
  }

  /**
   * test void ptr
   */
  @Test
  void testVoidPtr() throws RTException, ExecutionException, InterruptedException {
    RTValue voidPtrV = new RTValue(723123231L, RTValueType.VOIDPTR);
    rtObject.set("vptr", voidPtrV).get();
    RTValue rpcValue = rtObject.get("vptr").get();
    assertEquals(voidPtrV.getValue(), rpcValue.getValue());
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

  /**
   * check function is equals
   */
  private boolean checkEqualsFunction(RTValue v1, RTValue v2) {
    return ((RTFunction) v1.getValue()).getFunctionName()
        .equals(((RTFunction) v2.getValue()).getFunctionName());
  }

  /**
   * floating point values can be off by a little bit, so they may not report as exactly equal.
   * so i need use eps to check equal
   */
  private boolean checkEqualsFloat(Object v1, Object v2) {
    float eps = 0.001f;
    if (Math.abs((float) v1 - (float) v2) < eps) {
      return true;
    }
    return false;
  }

  /**
   * double  values can be off by a little bit, so they may not report as exactly equal.
   */
  private boolean checkEqualsDouble(Object v1, Object v2) {
    double eps = 0.0001;
    if (Math.abs((double) v1 - (double) v2) < eps) {
      return true;
    }
    return false;
  }


}
