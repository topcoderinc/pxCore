package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.StringReader;
import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import javax.json.Json;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTConst;
import org.pxscene.rt.remote.RTRemoteMessageType;
import org.pxscene.rt.remote.RTRemoteSerializer;
import org.pxscene.rt.remote.messages.RTMessageCallMethodRequest;
import org.pxscene.rt.remote.messages.RTMessageCallMethodResponse;
import org.pxscene.rt.remote.messages.RTMessageGetPropertyByNameRequest;
import org.pxscene.rt.remote.messages.RTMessageGetPropertyByNameResponse;
import org.pxscene.rt.remote.messages.RTMessageKeepAliveRequest;
import org.pxscene.rt.remote.messages.RTMessageKeepAliveResponse;
import org.pxscene.rt.remote.messages.RTMessageLocate;
import org.pxscene.rt.remote.messages.RTMessageOpenSessionRequest;
import org.pxscene.rt.remote.messages.RTMessageOpenSessionResponse;
import org.pxscene.rt.remote.messages.RTMessageSearch;
import org.pxscene.rt.remote.messages.RTMessageSetPropertyByNameRequest;
import org.pxscene.rt.remote.messages.RTMessageSetPropertyByNameResponse;

/**
 * The junit examples of remote serializer
 */
public class RTRemoteSerializerTest {

  private RTRemoteSerializer serializer;

  @BeforeEach
  void init() {
    serializer = new RTRemoteSerializer();
  }

  /**
   * test serializer from RTValue to JsonObject
   */
  @Test
  void testIntToJson() {
    RTValue rtValue = new RTValue(100);
    JsonObject jsonObject = RTRemoteSerializer.valueToJson(rtValue);
    JsonObjectBuilder builder = Json.createObjectBuilder();
    builder.add(RTConst.TYPE, RTValueType.INT32.getTypeCode());
    builder.add(RTConst.VALUE, 100);
    assertEquals(builder.build().toString(), jsonObject.toString());
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
   * test serializer RTMessageCallMethodRequest
   */
  @Test
  void testSerializerRTMessageCallMethodRequest() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageCallMethodRequest request = new RTMessageCallMethodRequest();
    request.setCorrelationKey(key);
    request.setObjectId("test_obj");
    request.setMethodName("methodName");
    List<RTValue> rtValueList = new ArrayList<>();
    rtValueList.add(new RTValue(1));
    request.setFunctionArgs(rtValueList);

    String jsonStr = new String(serializer.toBytes(request), RTRemoteSerializer.CHARSET);
    JsonObject jsonObject = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObject.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObject.getString(RTConst.FUNCTION_KEY), "methodName");
    assertEquals(jsonObject.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObject.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.METHOD_CALL_REQUEST.toString());
    assertEquals(jsonObject.getJsonArray(RTConst.FUNCTION_ARGS)
        .get(0).asJsonObject().getInt(RTConst.VALUE), 1);
  }

  /**
   * test deserializer RTMessageCallMethodRequest
   */
  @Test
  void testDeserializerRTMessageCallMethodRequest() throws RTException {
    String jsonStr = "{\"message.type\":\"method.call.request\",\"correlation.key\":\"ad508157-12f8-4df5-a85e-26b659344a2f\",\"object.id\":\"test_obj\",\"function.name\":\"methodName\",\"function.args\":[{\"type\":52,\"value\":1}]}";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageCallMethodRequest request = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(request.getObjectId(), "test_obj");
    assertEquals(request.getMethodName(), "methodName");
    assertEquals(request.getCorrelationKey(), "ad508157-12f8-4df5-a85e-26b659344a2f");
    assertEquals(request.getFunctionArgs().get(0).getValue(), 1);
    assertEquals(request.getMessageType(),
        RTRemoteMessageType.METHOD_CALL_REQUEST);
  }

  /**
   * test serializer RTMessageCallMethodResponse
   */
  @Test
  void testSerializerRTMessageCallMethodResponse() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageCallMethodResponse response = new RTMessageCallMethodResponse();
    response.setCorrelationKey(key);
    response.setValue(new RTValue(1));
    response.setStatus(new RTStatus(RTStatusCode.OK));

    String jsonStr = new String(serializer.toBytes(response), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(RTStatusCode.fromInt(jsonObj.getInt(RTConst.STATUS_CODE)), RTStatusCode.OK);
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getJsonObject(RTConst.FUNCTION_RETURN_VALUE).getInt(RTConst.VALUE), 1);
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.METHOD_CALL_RESPONSE.toString());
  }

  /**
   * test deserializer RTMessageCallMethodResponse
   */
  @Test
  void testDeserializerRTMessageCallMethodResponse() throws RTException {
    String jsonStr = "{\"message.type\":\"method.call.response\",\"correlation.key\":\"640fe030-5599-4aad-b554-260ac60e1753\",\"status.code\":0,\"function.return_value\":{\"type\":52,\"value\":1}}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageCallMethodResponse response = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(response.getStatus().getCode(), RTStatusCode.OK);
    assertEquals(response.getCorrelationKey(), "640fe030-5599-4aad-b554-260ac60e1753");
    assertEquals(response.getValue().getValue(), 1);
    assertEquals(response.getMessageType(),
        RTRemoteMessageType.METHOD_CALL_RESPONSE);
  }

  /**
   * test serializer RTMessageGetPropertyByNameRequest
   */
  @Test
  void testSerializerRTMessageGetPropertyByNameRequest() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageGetPropertyByNameRequest m = new RTMessageGetPropertyByNameRequest();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setPropertyName("prop01");
    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getString(RTConst.PROPERTY_NAME), "prop01");
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.GET_PROPERTY_BYNAME_REQUEST.toString());
  }

  /**
   * test deserializer RTMessageGetPropertyByNameRequest
   */
  @Test
  void testDeserializerRTMessageGetPropertyByNameRequest() throws RTException {
    String jsonStr = "{\"message.type\":\"get.byname.request\",\"correlation.key\":\"af710ce2-e0e9-4e65-ae2a-462e0f0a03e2\",\"property.name\":\"prop01\",\"object.id\":\"test_obj\"}";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageGetPropertyByNameRequest m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getObjectId(), "test_obj");
    assertEquals(m.getCorrelationKey(), "af710ce2-e0e9-4e65-ae2a-462e0f0a03e2");
    assertEquals(m.getPropertyName(), "prop01");
    assertEquals(m.getMessageType(),
        RTRemoteMessageType.GET_PROPERTY_BYNAME_REQUEST);
  }

  /**
   * test serializer RTMessageGetPropertyByNameResponse
   */
  @Test
  void testSerializerRTMessageGetPropertyByNameResponse() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageGetPropertyByNameResponse m = new RTMessageGetPropertyByNameResponse();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setStatus(new RTStatus(RTStatusCode.OK));
    m.setValue(new RTValue(1));

    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getJsonObject(RTConst.VALUE).getInt(RTConst.VALUE), 1);
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.GET_PROPERTY_BYNAME_RESPONSE.toString());
  }

  /**
   * test deserializer RTMessageGetPropertyByNameResponse
   */
  @Test
  void testDeserializerRTMessageGetPropertyByNameResponse() throws RTException {
    String jsonStr = "{\"message.type\":\"get.byname.response\",\"correlation.key\":\"a75c8c81-e187-4188-8f8c-3de9980e72d2\",\"object.id\":\"test_obj\",\"value\":{\"type\":52,\"value\":1},\"status.code\":0}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageGetPropertyByNameResponse m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getObjectId(), "test_obj");
    assertEquals(m.getCorrelationKey(), "a75c8c81-e187-4188-8f8c-3de9980e72d2");
    assertEquals(m.getValue().getValue(), 1);
    assertEquals(m.getMessageType(),
        RTRemoteMessageType.GET_PROPERTY_BYNAME_RESPONSE);
  }

  /**
   * test serializer RTMessageKeepAliveRequest
   */
  @Test
  void testSerializerRTMessageKeepAliveRequest() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageKeepAliveRequest m = new RTMessageKeepAliveRequest();
    m.setCorrelationKey(key);
    //compile error
    //String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    fail("there is should a serializer to serialize RTMessageKeepAliveRequest, but not found");
  }

  /**
   * test deserializer RTMessageKeepAliveRequest
   */
  @Test
  void testDeserializerRTMessageKeepAliveRequest() throws RTException {
    String jsonStr = "{\"message.type\":\"keep_alive.request\",\"correlation.key\":\"a75c8c81-e187-4188-8f8c-3de9980e72d2\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageKeepAliveRequest m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getMessageType(), RTRemoteMessageType.KEEP_ALIVE_REQUEST);
    assertEquals(m.getCorrelationKey(), "a75c8c81-e187-4188-8f8c-3de9980e72d2");
  }

  /**
   * test serializer RTMessageKeepAliveResponse
   */
  @Test
  void testSerializerRTMessageKeepAliveResponse() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageKeepAliveResponse m = new RTMessageKeepAliveResponse();
    m.setCorrelationKey(key);
    //compile error
    //String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    fail("there is should a serializer to serialize RTMessageKeepAliveResponse, but not found");
  }

  /**
   * test deserializer RTMessageKeepAliveResponse
   */
  @Test
  void testDeserializerRTMessageKeepAliveResponse() throws RTException {
    String jsonStr = "{\"message.type\":\"keep_alive.response\",\"correlation.key\":\"a75c8c81-e187-4188-8f8c-3de9980e72d2\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    try {
      RTMessageKeepAliveRequest m = serializer.fromBytes(bytes, 0, bytes.length);
      assertEquals(m.getMessageType(), RTRemoteMessageType.KEEP_ALIVE_RESPONSE);
      assertEquals(m.getCorrelationKey(), "a75c8c81-e187-4188-8f8c-3de9980e72d2");
    } catch (Exception e) {
      fail("there should no exception, but got " + e.getMessage());
    }
  }

  /**
   * test serializer RTMessageLocate
   */
  @Test
  void testSerializerRTMessageLocate() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageLocate m = new RTMessageLocate();
    URI uri = URI.create("udp://127.0.0.1:3434");
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setSenderId(0);
    m.setEndpoint(uri);

    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    try {
      assertEquals(jsonObj.getInt(RTConst.SENDER_ID), 0);
    } catch (Exception e) {
      fail("there should no exception, but got " + e.getMessage());
    }
    assertEquals(jsonObj.getString(RTConst.REPLY_TO), uri.toString());
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.LOCATE_OBJECT.toString());
  }

  /**
   * test deserializer RTMessageLocate
   */
  @Test
  void testDeserializerRTMessageLocate() throws RTException {
    String jsonStr = "{\"message.type\":\"locate\",\"correlation.key\":\"296215ab-7dd1-41cf-93c6-8946af0483bb\",\"object.id\":\"test_obj\",\"endpoint\":\"udp://127.0.0.1:3434\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageLocate m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getObjectId(), "test_obj");
    assertEquals(m.getCorrelationKey(), "296215ab-7dd1-41cf-93c6-8946af0483bb");
    assertEquals(m.getEndpoint().toString(), "udp://127.0.0.1:3434");
    assertEquals(m.getMessageType(),
        RTRemoteMessageType.LOCATE_OBJECT);
  }


  /**
   * test serializer RTMessageOpenSessionRequest
   */
  @Test
  void testSerializerRTMessageOpenSessionRequest() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageOpenSessionRequest m = new RTMessageOpenSessionRequest();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");

    //compile error
    //String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    fail("there is should a serializer to serialize RTMessageOpenSessionRequest, but not found");
  }

  /**
   * test deserializer RTMessageOpenSessionRequest
   */
  @Test
  void testDeserializerRTMessageOpenSessionRequest() throws RTException {
    String jsonStr = "{\"message.type\":\"session.open.request\",\"correlation.key\":\"a75c8c81-e187-4188-8f8c-3de9980e72d2\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    try {
      RTMessageOpenSessionRequest m = serializer.fromBytes(bytes, 0, bytes.length);
      assertEquals(m.getMessageType(), RTRemoteMessageType.SESSION_OPEN_REQUEST);
      assertEquals(m.getCorrelationKey(), "a75c8c81-e187-4188-8f8c-3de9980e72d2");
    } catch (Exception e) {
      fail("there should no exception, but got " + e.getMessage());
    }
  }

  /**
   * test serializer RTMessageOpenSessionResponse
   */
  @Test
  void testSerializerRTMessageOpenSessionResponse() throws RTException {
    String key = UUID.randomUUID().toString();
    RTMessageOpenSessionResponse m = new RTMessageOpenSessionResponse();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");

    //compile error
    // String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    fail("there is should a serializer to serialize RTMessageOpenSessionResponse, but not found");
  }

  /**
   * test deserializer RTMessageOpenSessionResponse
   */
  @Test
  void testDeserializerRTMessageOpenSessionResponse() throws RTException {
    String jsonStr = "{\"message.type\":\"session.open.response\",\"correlation.key\":\"a75c8c81-e187-4188-8f8c-3de9980e72d2\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    try {
      RTMessageOpenSessionResponse m = serializer.fromBytes(bytes, 0, bytes.length);
      assertEquals(m.getMessageType(), RTRemoteMessageType.SESSION_OPEN_RESPIONSE);
      assertEquals(m.getCorrelationKey(), "a75c8c81-e187-4188-8f8c-3de9980e72d2");
    } catch (Exception e) {
      fail("there should no exception, but got " + e.getMessage());
    }
  }


  /**
   * test serializer RTMessageSearch
   */
  @Test
  void testSerializerRTMessageSearch() throws RTException {
    String key = UUID.randomUUID().toString();
    URI uri = URI.create("tcp://127.0.0.1:3434");

    RTMessageSearch m = new RTMessageSearch();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setReplyTo(uri);
    m.setSenderId(1);

    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getInt(RTConst.SENDER_ID), 1);
    assertEquals(jsonObj.getString(RTConst.REPLY_TO), uri.toString());
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.SERACH_OBJECT.toString());
  }


  /**
   * test deserializer RTMessageSearch
   */
  @Test
  void testDeserializerRTMessageSearch() throws RTException {
    String jsonStr = "{\"message.type\":\"search\",\"correlation.key\":\"4a4a0fb4-99e6-4b5a-86dc-d7e468a44eba\",\"object.id\":\"test_obj\",\"sender.id\":1,\"reply-to\":\"tcp://127.0.0.1:3434\"}\n";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageSearch m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getObjectId(), "test_obj");
    assertEquals(m.getCorrelationKey(), "4a4a0fb4-99e6-4b5a-86dc-d7e468a44eba");
    assertEquals(m.getSenderId(), 1);
    assertEquals(m.getReplyTo().toString(), "tcp://127.0.0.1:3434");
    assertEquals(m.getMessageType(), RTRemoteMessageType.SERACH_OBJECT);
  }

  /**
   * test serializer RTMessageSetPropertyByNameRequest
   */
  @Test
  void testSerializerRTMessageSetPropertyByNameRequest() throws RTException {
    String key = UUID.randomUUID().toString();

    RTMessageSetPropertyByNameRequest m = new RTMessageSetPropertyByNameRequest();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setPropertyName("prop1");
    m.setValue(new RTValue(1));

    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getString(RTConst.PROPERTY_NAME), "prop1");
    assertEquals(jsonObj.getJsonObject(RTConst.VALUE).getInt(RTConst.VALUE), 1);
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.SET_PROPERTY_BYNAME_REQUEST.toString());
  }

  /**
   * test deserializer RTMessageSetPropertyByNameRequest
   */
  @Test
  void testDeserializerRTMessageSetPropertyByNameRequest() throws RTException {
    String jsonStr = "{\"message.type\":\"set.byname.request\",\"correlation.key\":\"7237bed5-a235-4333-a53a-0a7475740800\",\"object.id\":\"test_obj\",\"property.name\":\"prop1\",\"value\":{\"type\":52,\"value\":1}}";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    RTMessageSetPropertyByNameRequest m = serializer.fromBytes(bytes, 0, bytes.length);
    assertEquals(m.getObjectId(), "test_obj");
    assertEquals(m.getCorrelationKey(), "7237bed5-a235-4333-a53a-0a7475740800");
    assertEquals(m.getValue().getValue(), 1);
    assertEquals(m.getPropertyName(), "prop1");
    assertEquals(m.getMessageType(), RTRemoteMessageType.SET_PROPERTY_BYNAME_REQUEST);
  }

  /**
   * test serializer RTMessageSetPropertyByNameResponse
   */
  @Test
  void testSerializerRTMessageSetPropertyByNameResponse() throws RTException {
    String key = UUID.randomUUID().toString();

    RTMessageSetPropertyByNameResponse m = new RTMessageSetPropertyByNameResponse();
    m.setCorrelationKey(key);
    m.setObjectId("test_obj");
    m.setStatusCode(RTStatusCode.OK);
    String jsonStr = new String(serializer.toBytes(m), RTRemoteSerializer.CHARSET);
    JsonObject jsonObj = Json.createReader(new StringReader(jsonStr)).readObject();
    assertEquals(jsonObj.getString(RTConst.OBJECT_ID_KEY), "test_obj");
    assertEquals(jsonObj.getString(RTConst.CORRELATION_KEY), key);
    assertEquals(jsonObj.getInt(RTConst.STATUS_CODE), RTStatusCode.OK.getCode());
    assertEquals(jsonObj.getString(RTConst.MESSAGE_TYPE),
        RTRemoteMessageType.SET_PROPERTY_BYNAME_RESPONSE.toString());
  }

  /**
   * test deserializer RTMessageSetPropertyByNameResponse
   */
  @Test
  void testDeserializerRTMessageSetPropertyByNameResponse() throws RTException {
    String jsonStr = "{\"message.type\":\"set.byname.response\",\"correlation.key\":\"c37b1796-287f-4c86-af04-ff17a98e2c68\",\"object.id\":\"test_obj\",\"status.code\":0}";
    byte[] bytes = jsonStr.getBytes(RTRemoteSerializer.CHARSET);
    try {
      RTMessageSetPropertyByNameResponse m = serializer.fromBytes(bytes, 0, bytes.length);
      assertEquals(m.getObjectId(), "test_obj");
      assertEquals(m.getCorrelationKey(), "c37b1796-287f-4c86-af04-ff17a98e2c68");
      assertEquals(m.getStatusCode(), RTStatusCode.OK);
      assertEquals(m.getMessageType(), RTRemoteMessageType.SET_PROPERTY_BYNAME_RESPONSE);
    } catch (Exception e) {
      fail("there should not be has exception, but got " + e.getMessage());
    }
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
