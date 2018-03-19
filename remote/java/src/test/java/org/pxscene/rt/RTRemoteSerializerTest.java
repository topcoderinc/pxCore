package org.pxscene.rt;

import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTConst;
import org.pxscene.rt.remote.RTRemoteSerializer;

import javax.json.Json;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * The junit examples of remote serializer
 */
public class RTRemoteSerializerTest {
  /**
   * test serializer from RTValue to JsonObject
   */
  @Test
  void testIntToJson() {
    RTValue rtValue = new RTValue(100);
    JsonObject jsonObject = RTRemoteSerializer.valueToJson(rtValue);
    assertEquals("{'type':52,'value':100}", jsonObject.toString());
  }


  /**
   * test serializer from JsonObject to RTValue
   */
  @Test
  void testJsonToInt() {
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.INT32.getTypeCode());
    builder.add(RTConst.VALUE, 100);
    RTValue rtValue = RTRemoteSerializer.valueFromJson(builder.build());
    assertEquals(rtValue.getValue(), 100);
    assertEquals(rtValue.getType().getTypeCode(), RTValueType.INT32.getTypeCode());
  }

  /**
   * test serializer from JsonObject to RTValue
   */
  @Test
  void testJsonToFloat() {
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.FLOAT.getTypeCode());
    builder.add(RTConst.VALUE, 100.01f);
    RTValue rtValue = RTRemoteSerializer.valueFromJson(builder.build());
    assertEquals(true, checkEqualsFloat(rtValue.getValue(), 100.01f));
    assertEquals(rtValue.getType().getTypeCode(), RTValueType.FLOAT.getTypeCode());
  }

  /**
   * test serializer from RTValue to JsonObject
   */
  @Test
  void testFloatToJson() {
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.FLOAT.getTypeCode());
    builder.add(RTConst.VALUE, 100.01f);

    RTValue rtValue = new RTValue(100.01f);
    JsonObject jsonObject = RTRemoteSerializer.valueToJson(rtValue);
    assertEquals(builder.build().toString(), jsonObject.toString());
  }

  /**
   * test serializer from JsonObject to RTValue
   */
  @Test
  void testJsonToDouble() {
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.DOUBLE.getTypeCode());
    builder.add(RTConst.VALUE, 100.013);
    RTValue rtValue = RTRemoteSerializer.valueFromJson(builder.build());
    assertEquals(true, checkEqualsDouble(rtValue.getValue(), 100.013));
    assertEquals(rtValue.getType().getTypeCode(), RTValueType.DOUBLE.getTypeCode());
  }

  /**
   * test serializer from RTValue to JsonObject
   */
  @Test
  void testDoubleToJson() {
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.DOUBLE.getTypeCode());
    builder.add(RTConst.VALUE, 100.013);

    RTValue rtValue = new RTValue(100.013);
    JsonObject jsonObject = RTRemoteSerializer.valueToJson(rtValue);
    assertEquals(builder.build().toString(), jsonObject.toString());
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
