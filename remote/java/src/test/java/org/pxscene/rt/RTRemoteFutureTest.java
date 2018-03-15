package org.pxscene.rt;


import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.concurrent.ExecutionException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.pxscene.rt.remote.RTRemoteFuture;

/**
 * The junit examples of RTRemoteFuture.
 */
public class RTRemoteFutureTest {


  RTRemoteFuture future;

  /**
   * create new RTRemoteFuture for each test
   */
  @BeforeEach
  void init() {
    future = new RTRemoteFuture("key-02");
  }

  /**
   * test cancel future
   */
  @Test
  void testRTRemoteFutureCalcel() {
    assertEquals(future.getCorrelationKey(), "key-02");
    future.cancel(true);
    assertEquals(future.isCancelled(), true);
  }

  /**
   * test future done
   */
  @Test
  void testRTRemoteFutureDone() throws ExecutionException, InterruptedException {
    future.complete("hello", new RTStatus(RTStatusCode.OK));
    assertEquals(future.get(), "hello");
    assertEquals(future.isDone(), true);
  }

  /**
   * test get futrue
   */
  @Test
  void testRTRemoteFutureGet() throws ExecutionException, InterruptedException {
//    try {
//      future.get(10, TimeUnit.MILLISECONDS);
//    } catch (Exception e) {
//      assertEquals(e.getMessage(), "error waiting for response");
//    }
    // TODO this should be return timeout error after 10ms, but is always block the thread and didn't return
    assertEquals(false, true);
  }
}
