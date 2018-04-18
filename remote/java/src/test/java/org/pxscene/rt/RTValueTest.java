package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;

import examples.TestObject;
import java.math.BigInteger;
import org.junit.jupiter.api.Test;

/**
 * The junit examples of RTValue.
 */
public class RTValueTest {


  /**
   * test set type
   */
  @Test
  void testSetType() {
    int v = 1023;
    RTValue rtValue = new RTValue();
    rtValue.setValue(v);
    rtValue.setType(RTValueType.INT32);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.INT32, rtValue.getType());
  }

  /**
   * test int 32
   */
  @Test
  void testInt() {
    int v = 1023;
    RTValue rtValue = new RTValue(v);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.INT32, rtValue.getType());
  }

  /**
   * test uint 32
   */
  @Test
  void testUint32() {
    Long v = 4294967295L;
    RTValue rtValue = new RTValue(v, RTValueType.UINT32);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.UINT32, rtValue.getType());
  }

  /**
   * test void type
   */
  @Test
  void testVoid() {
    RTValue rtValue = new RTValue(null, RTValueType.VOID);
    assertEquals(null, rtValue.getValue());
    assertEquals(RTValueType.VOID, rtValue.getType());
  }

  /**
   * test value type
   */
  @Test
  void testRTValue() {
    RTValue v = new RTValue(1);
    RTValue rtValue = new RTValue(v, RTValueType.VALUE);
    assertEquals(v, rtValue.getValue());
    assertEquals(v.hashCode(), rtValue.getValue().hashCode());
    assertEquals(RTValueType.VALUE, rtValue.getType());
  }

  /**
   * test int 8
   */
  @Test
  void testInt8() {
    byte v = -128;
    RTValue rtValue = new RTValue(v, RTValueType.INT8);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.INT8, rtValue.getType());
  }

  /**
   * test uint8
   */
  @Test
  void testUint8() {
    short v = 255;
    RTValue rtValue = new RTValue(v, RTValueType.UINT8);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.UINT8, rtValue.getType());
  }

  /**
   * test boolean
   */
  @Test
  void testBoolean() {
    boolean v = false;
    RTValue rtValue = new RTValue(v, RTValueType.BOOLEAN);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.BOOLEAN, rtValue.getType());
  }

  /**
   * test int 64
   */
  @Test
  void testInt64() {
    long v = -9223372036854775808L;
    RTValue rtValue = new RTValue(v, RTValueType.INT64);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.INT64, rtValue.getType());
  }

  /**
   * test uint 64
   */
  @Test
  void testUint64() {
    BigInteger v = new BigInteger("18446744073709551615");
    RTValue rtValue = new RTValue(v, RTValueType.UINT64);
    assertEquals(v, rtValue.getValue());
    assertEquals(v.hashCode(), rtValue.getValue().hashCode());
    assertEquals(RTValueType.UINT64, rtValue.getType());
  }

  /**
   * test void ptr , void ptr is long in java version
   */
  @Test
  void testVoidPtr() {
    Long v = 4294967295L;
    RTValue rtValue = new RTValue(v, RTValueType.VOIDPTR);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.VOIDPTR, rtValue.getType());
  }

  /**
   * test float
   */
  @Test
  void testFloat() {
    float v = 1234.123f;
    RTValue rtValue = new RTValue(v);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.FLOAT, rtValue.getType());
  }

  /**
   * test double
   */
  @Test
  void testDouble() {
    double v = 1234.12345123;
    RTValue rtValue = new RTValue(v);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.DOUBLE, rtValue.getType());
  }

  /**
   * test string
   */
  @Test
  void testString() {
    String v = "hello world !";
    RTValue rtValue = new RTValue(v);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.STRING, rtValue.getType());
  }

  /**
   * test rtObject
   */
  @Test
  void testObject() {
    TestObject testObject = new TestObject("testObj");
    RTValue rtValue = new RTValue(testObject);
    assertEquals(testObject, rtValue.getValue());
    assertEquals(testObject.hashCode(), rtValue.getValue().hashCode());
    assertEquals(RTValueType.OBJECT, rtValue.getType());
  }

  /**
   * test rtFunction
   */
  @Test
  void testFunction() {
    RTFunction rtFunction = new RTFunction(rtValueList -> {

    });
    RTValue rtValue = new RTValue(rtFunction);
    assertEquals(rtFunction, rtValue.getValue());
    assertEquals(rtFunction.hashCode(), rtValue.getValue().hashCode());
    assertEquals(RTValueType.FUNCTION, rtValue.getType());
  }


  /**
   * test void and type
   */
  @Test
  void testRtValueType() {
    char code = '\0';
    RTValueType type = RTValueType.fromTypeCode(code);
    assertEquals(code, type.getTypeCode());
    assertEquals(RTValueType.VOID, RTValueType.fromTypeCode('n')); // unknown -> void
  }
}
