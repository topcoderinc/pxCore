package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;

import examples.TestObject;
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
   * test int
   */
  @Test
  void testInt() {
    int v = 1023;
    RTValue rtValue = new RTValue(v);
    assertEquals(v, rtValue.getValue());
    assertEquals(RTValueType.INT32, rtValue.getType());
  }

  /**
   * test float
   */
  @Test
  void testFloat() {
    float v = 1234.123f;
    assertEquals(v, new RTValue(v).getValue());
  }

  /**
   * test double
   */
  @Test
  void testDouble() {
    double v = 1234.12345123;
    assertEquals(v, new RTValue(v).getValue());
  }

  /**
   * test string
   */
  @Test
  void testString() {
    String v = "hello world !";
    assertEquals(v, new RTValue(v).getValue());
  }

  /**
   * test rtObject
   */
  @Test
  void testObject() {
    TestObject testObject = new TestObject("testObj");
    assertEquals(testObject, new RTValue(testObject).getValue());
  }

  /**
   * test rtFunction
   */
  @Test
  void testFunction() {
    RTFunction rtFunction = new RTFunction(rtValueList -> {

    });
    assertEquals(rtFunction, new RTValue(rtFunction).getValue());
  }


  /**
   * test void and type
   */
  @Test
  void testRtValueType() {
    char code = '\0';
    RTValueType type = RTValueType.fromTypeCode(code);
    assertEquals(code, type.getTypeCode());
    assertEquals(RTValueType.VOID, RTValueType.fromTypeCode('n')); // unkown -> void
  }

}
