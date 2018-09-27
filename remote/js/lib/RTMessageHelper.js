/**
 * Copyright (C) 2018 TopCoder Inc., All Rights Reserved.
 */

/**
 * The rt remote message helper, used to create message object
 *
 * @author      TCSCODER
 * @version     1.0
 */

const RTConst = require('./RTConst');
const helper = require('./common/helper');
const RTRemoteMessageType = require('./RTRemoteMessageType');
const RTValueHelper = require('./RTValueHelper');
const RTValueType = require('./RTValueType');

/**
 * create new locate object message
 * @param {string} objectId  the object id
 * @param {string} replyTo the udp reply endpoint
 * @returns {object} the locate message
 */
function newLocateRequest(objectId, replyTo) {
  const locateObj = {};
  locateObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.SEARCH_OBJECT;
  locateObj[RTConst.CORRELATION_KEY] = helper.getRandomUUID();
  locateObj[RTConst.OBJECT_ID_KEY] = objectId;
  locateObj[RTConst.SENDER_ID] = 0;
  locateObj[RTConst.REPLY_TO] = replyTo;
  return locateObj;
}

/**
 * create new locate object response message
 * @param {string} endpoint
 * @param {string} objectId
 * @param {number|int} senderId
 * @param {string} correlationKey
 * @return {object} the locate object response message
 */
function newLocateResponse(endpoint, objectId, senderId, correlationKey) {
  const response = {};
  response[RTConst.ENDPOINT] = endpoint;
  response[RTConst.OBJECT_ID_KEY] = objectId;
  response[RTConst.SENDER_ID] = senderId;
  response[RTConst.CORRELATION_KEY] = correlationKey;
  response[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.LOCATE_OBJECT;
  return response;
}

/**
 * create set property by name request message
 * @param {string} objectId the object id
 * @param {string|null} propName the property name
 * @param {object} rtValue the set value
 * @return {object} the set request message
 */
function newSetRequest(objectId, propName, rtValue) {
  const setRequestObj = {};
  setRequestObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.SET_PROPERTY_BYNAME_REQUEST;
  setRequestObj[RTConst.CORRELATION_KEY] = helper.getRandomUUID();
  setRequestObj[RTConst.OBJECT_ID_KEY] = objectId;
  if (propName) {
    setRequestObj[RTConst.PROPERTY_NAME] = propName;
  }
  setRequestObj[RTConst.VALUE] = rtValue;
  return setRequestObj;
}

/**
 * create new get property by name request message
 * @param {string} objectId the object id
 * @param {string|null} propName the property name
 * @return {object} the get property message
 */
function newGetRequest(objectId, propName) {
  const getRequestObj = {};
  getRequestObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.GET_PROPERTY_BYNAME_REQUEST;
  getRequestObj[RTConst.CORRELATION_KEY] = helper.getRandomUUID();
  getRequestObj[RTConst.OBJECT_ID_KEY] = objectId;
  if (propName) {
    getRequestObj[RTConst.PROPERTY_NAME] = propName;
  }
  return getRequestObj;
}

/**
 * create new call request message
 * @param {string} objectId objectId the object id
 * @param {string} methodName the method name
 * @param {array} args the call request args
 * @return {object} the call request message
 */
function newCallMethodRequest(objectId, methodName, ...args) {
  const callRequestObj = {};
  callRequestObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.METHOD_CALL_REQUEST;
  callRequestObj[RTConst.CORRELATION_KEY] = helper.getRandomUUID();
  callRequestObj[RTConst.OBJECT_ID_KEY] = objectId;
  callRequestObj[RTConst.FUNCTION_KEY] = methodName;
  callRequestObj[RTConst.FUNCTION_ARGS] = args;
  return callRequestObj;
}

/**
 * create new call response message
 * @param {string} correlationKey the call request correlation key
 * @param {object} returnValue the call response value
 * @param {RTStatusCode|number} statusCode the status code
 * @return {object} the call response message
 */
function newCallResponse(correlationKey, returnValue, statusCode) {
  const callResponseObj = {};
  callResponseObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.METHOD_CALL_RESPONSE;
  callResponseObj[RTConst.CORRELATION_KEY] = correlationKey;
  callResponseObj[RTConst.STATUS_CODE] = statusCode;
  callResponseObj[RTConst.FUNCTION_RETURN_VALUE] = returnValue || RTValueHelper.create(null, RTValueType.VOID);
  return callResponseObj;
}

/**
 * create new keep alive response message
 * @param {string} correlationKey the keep alive request correlation key
 * @param {RTStatusCode|number} statusCode the status code
 * @return {object} the keep alive response message
 */
function newKeepAliveResponse(correlationKey, statusCode) {
  const keepAliveReponseObj = {};
  keepAliveReponseObj[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.KEEP_ALIVE_RESPONSE;
  keepAliveReponseObj[RTConst.CORRELATION_KEY] = correlationKey;
  keepAliveReponseObj[RTConst.STATUS_CODE] = statusCode;
  return keepAliveReponseObj;
}

/**
 * create new set property response message
 * @param {string} correlationKey the request correlation key
 * @param {RTStatusCode|number} statusCode the status code
 * @param {string} objectId the object id
 * @return {object} the response message
 */
function newSetPropertyResponse(correlationKey, statusCode, objectId) {
  const setResponse = {};
  setResponse[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.SET_PROPERTY_BYNAME_RESPONSE;
  setResponse[RTConst.CORRELATION_KEY] = correlationKey;
  setResponse[RTConst.STATUS_CODE] = statusCode;
  setResponse[RTConst.OBJECT_ID_KEY] = objectId;
  return setResponse;
}

/**
 * create new get property response message
 * @param {string} correlationKey the request correlation key
 * @param {RTStatusCode|number} statusCode the status code
 * @param {string} objectId the object id
 * @param {object} value the rtValue
 * @return {object} the response message
 */
function newGetPropertyResponse(correlationKey, statusCode, objectId, value) {
  const getResponse = {};
  getResponse[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.GET_PROPERTY_BYNAME_RESPONSE;
  getResponse[RTConst.CORRELATION_KEY] = correlationKey;
  getResponse[RTConst.STATUS_CODE] = statusCode;
  getResponse[RTConst.OBJECT_ID_KEY] = objectId;
  getResponse[RTConst.VALUE] = value;
  return getResponse;
}

/**
 * create new get open session response message
 * @param {string} correlationKey the request correlation key
 * @param {string} objectId the object id
 * @return {object} the response message
 */
function newOpenSessionResponse(correlationKey, objectId) {
  const response = {};
  response[RTConst.MESSAGE_TYPE] = RTRemoteMessageType.SESSION_OPEN_RESPIONSE;
  response[RTConst.CORRELATION_KEY] = correlationKey;
  response[RTConst.OBJECT_ID_KEY] = objectId;
  return response;
}

module.exports = {
  newLocateRequest,
  newLocateResponse,
  newSetRequest,
  newGetRequest,
  newCallMethodRequest,
  newCallResponse,
  newKeepAliveResponse,
  newSetPropertyResponse,
  newGetPropertyResponse,
  newOpenSessionResponse,
};
