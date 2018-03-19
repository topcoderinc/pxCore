package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertLinesMatch;

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
