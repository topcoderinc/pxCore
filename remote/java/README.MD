# Hercules RT Remote - Java

### Platform and tools

- ubuntu 16.04

### Download pxCore code

-  `git clone https://github.com/topcoderinc/pxCore.git`
-  `cd pxCore`



### Build remote

* Install dependencies,  `sudo apt-get install git libglew-dev freeglut3 freeglut3-dev libgcrypt11-dev zlib1g-dev g++ libssl-dev nasm autoconf libtool cmake gnutls-bin libgnutls-dev autoconf libtool nasm uuid-dev maven quilt`
* Install java
   * `sudo add-apt-repository ppa:openjdk-r/ppa && sudo apt-get update`
   * `sudo apt-get install openjdk-8-jdk`
* Build dependencies,  `cd examples/pxScene2d/external` and then run `./build.sh`
* Build pxScene 
  * `cd ../../..`
  * `mkdir build && cd build`
  * `cmake .. -DSUPPORT_DUKTAPE=OFF && make -j1`
* Build rtRemote, `cd ../remote && make -j1` ,
* Build server example:
  * `cd ../rtRemoteSimpleServer`
  * `mkdir build && cd build`
  * `cmake .. && make`
  * **copy config file to build** **folder** `cp ../rtremote.conf ./`
* Run the sample server `./sample_server`



### Build java remote

- cd `pxCore/rtRemote/java` , `mvn compile`



### Validation

#### examples validation 

* startup server
  * **c++ sample server** , Follow *Build remote and run example* section to start up sample server
  * **java sample server**, Follow *Build java remote* to build java codes, then ``cd pxCore/rtRemote/java` use `mvn exec:java -Dexec.mainClass="examples.server.SampleServer"` to startup server.

* Basic client examples , cd `pxCore/rtRemote/java` , `mvn compile`
  * `mvn exec:java -Dexec.mainClass="examples.TypeTest"`, this example include all types test.
  * `mvn exec:java -Dexec.mainClass="examples.MethodTest"`, this example include a lot of method test.
  * `mvn exec:java -Dexec.mainClass="examples.ThreadTest"`, this example create 4 threads for each 10 seconds to test type and methods.

* run junits

  * run `mvn clean test`
  * open `target/jacoco-ut/index.html` , you can see the code coverage, for now the code coverage is 88%.

  notes: for now, there are 3 failures junit examples,  they are bugs, see the test results, i marked as **TODO** in junit codes.


#### loop to find object validation 

Loop to find object test , close server , then start `mvn exec:java -Dexec.mainClass="examples.TypeTest"`, you can see client is looping to find remote object, then start server, the client will start working.

#### IntelliJ project validation 

- download and install IntelliJ idea https://www.jetbrains.com/idea/
- open intelliJ idea, click open project, then choose folder *pxCore/rtRemote/java*, it should auto load the whole project to ide.



### RTValue types list

| C++ RTRemote type | Java Types |
| ----------------- | ---------- |
| bool              | Boolean    |
| float             | Float      |
| double            | Double     |
| int8_t            | Short      |
| uint8_t           | Short      |
| int32_t           | Integer    |
| uint32_t          | Long       |
| int64_t           | Long       |
| uint64_t          | BigInteger |
| rtString          | String     |
| rtObject          | RTObject   |
| rtFunctionRef     | RTFunction |
| voidPtr           | Long       |



## Notes

- c++ rtRemote server didn't suport define **void** type properties, so *TypeTest* ignored set void test

- c++ rtRemote server didn't implement **RT_valueType** type properties, so *TypeTest* ignored set/get **VALUE** test

- all client examples run about long time, like 10-30mins, c++ remote server crashed, c++ remote code had mem leak problem or something else ( **java server didn't have these errors**).

  ```
  terminate called after throwing an instance of 'std::bad_alloc'
    what():  std::bad_alloc
  Aborted (core dumped)
  ```
- only RTObject and RTFunction returned from server has the same pointer/ref  when client cached, other basic values all are new Object, the don't have the same pointer/ref.

## Failed Tests

1. RTHelperTest.testGetPropertyByNameWithInvalidType:50 this should not be raise Exception , should be return PROP_NOT_FOUND

2. RTHelperTest.testSetPropertyNameWithDifferentType:92 this should return ok, but is return  TYPE_MISMATCH ==> expected: <TYPE_MISMATCH> but was: <OK>

3. RTMethodTest.testUnknownMethod:119 this should not be raise Exception , should be return METHOD_NOT_FOUND

4. RTRemoteFutureTest.testRTRemoteFutureGet:59 this should be return timeout error after 10ms, but is always block the thread and did not return

5. RTRemoteSerializerTest.testDeserializerRTMessageKeepAliveResponse:321 there should no exception, but got cannot prase the message where type = keep_alive.response

   **reason**: *RTRemoteSerializer didn't implement prase the keep_alive.response type message.*

6. RTRemoteSerializerTest.testDeserializerRTMessageOpenSessionRequest:395 there should no exception, but got cannot prase the message where type = session.open.request

   **reason**: *RTRemoteSerializer didn't implement prase the session.open.request type message.*

7. RTRemoteSerializerTest.testDeserializerRTMessageOpenSessionResponse:426 there should no exception, but got cannot prase the message where type = session.open.response

   **reason**: *RTRemoteSerializer didn't implement prase the session.open.response type message.*

8. RTRemoteSerializerTest.testDeserializerRTMessageSetPropertyByNameResponse:543 there should not be has exception, but got org.pxscene.rt.remote.messages.RTMessageGetPropertyByNameResponse cannot be cast to org.pxscene.rt.remote.messages.RTMessageSetPropertyByNameResponse

   **reason**: *in RTRemoteSerializer method fromJson_SetPropertyByNameResponse,line 247 should return RTMessageSetPropertyByNameResponse , but is return RTMessageGetPropertyByNameResponse*

9. RTRemoteSerializerTest.testSerializerRTMessageKeepAliveRequest:281 there is should a serializer to serialize RTMessageKeepAliveRequest, but not found

   **reason**: *RTRemoteSerializer didn't implement serialize the RTMessageKeepAliveRequest message.*

10. RTRemoteSerializerTest.testSerializerRTMessageKeepAliveResponse:306 there is should a serializer to serialize RTMessageKeepAliveResponse, but not found

    **reason**: *RTRemoteSerializer didn't implement serialize the RTMessageKeepAliveResponse message.*

11. RTRemoteSerializerTest.testSerializerRTMessageLocate:345 there should no exception, but got null

    **reason**: *RTRemoteSerializer missing the sender id property when serialize the RTMessageLocate message, so it return null value when get the sender id from serialized jsonObject.*

12. RTRemoteSerializerTest.testSerializerRTMessageOpenSessionRequest:380 there is should a serializer to serialize RTMessageOpenSessionRequest, but not found

    **reason**: *RTRemoteSerializer didn't implement serialize the RTMessageOpenSessionRequest message.*

13. RTRemoteSerializerTest.testSerializerRTMessageOpenSessionResponse:411 there is should a serializer to serialize RTMessageOpenSessionResponse, but not found

    **reason**: *RTRemoteSerializer didn't implement serialize the RTMessageOpenSessionResponse message.*
