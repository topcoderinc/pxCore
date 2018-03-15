package org.pxscene.rt;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;


/**
 * the junit examples of RTStatus.
 */
public class RTStatusTest {


  /**
   * test status methods
   */
  @Test
  void testRtStatus() {
    RTStatus rtStatus = new RTStatus(RTStatusCode.OK, "succeed");
    assertEquals(rtStatus.getCode(), RTStatusCode.OK);
    assertEquals(rtStatus.toString(), "OK:succeed");
    rtStatus.setMessage("message");
    assertEquals(rtStatus.getMessage(), "message");
  }


}
