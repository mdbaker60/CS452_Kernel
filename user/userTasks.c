#include <bwio.h>
#include <clock.h>
#include <syscall.h>

void consumer() {
  int longMessage[16] = {0};
  int longReply[16] = {0};
  int shortMessage = 0;
  int shortReply = 0;
  int tid;

  Receive(&tid, (char *)longMessage, 64);
  Reply(tid, (char *)longReply, 64);
  bwprintf(COM2, "first message received\r");
  Receive(&tid, (char *)shortMessage, 4);
  Reply(tid, (char *)shortReply, 64);
  bwprintf(COM2, "second message recived\r");
  Receive(&tid, (char *)longMessage, 64);
  Reply(tid, (char *)longReply, 64);
  bwprintf(COM2, "third message received\r");
  Receive(&tid, (char *)shortMessage, 4);
  Reply(tid, (char *)shortReply, 64);
  bwprintf(COM2, "fourth message recived\r");
  Exit();
}

void producer_high() {
  int longMessage[16] = {0};
  int longReply[16] = {0};
  int shortMessage = 0;
  int shortReply = 0;

  int startTime = getClockTick();
  Send(2, (char *)longMessage, 64, (char *)longReply, 64);
  int endTime = getClockTick();
  int diff = startTime - endTime;
  if(diff < 0) diff += 508000;
  bwprintf(COM2, "64-byte(send first) test: %d ticks\r", diff);

  startTime = getClockTick();
  Send(2, (char *)shortMessage, 4, (char *)shortReply, 4);
  endTime = getClockTick();
  diff = startTime - endTime;
  if(diff < 0) diff += 508000;
  bwprintf(COM2, "4-byte(send first) test: %d ticks\r", diff);
  Exit();
}

void producer_low() {
  int longMessage[16] = {0};
  int longReply[16] = {0};
  int shortMessage = 0;
  int shortReply = 0;

  int startTime = getClockTick();
  Send(2, (char *)longMessage, 64, (char *)longReply, 64);
  int endTime = getClockTick();
  int diff = startTime - endTime;
  if(diff < 0) diff += 508000;
  bwprintf(COM2, "64-byte(receive first) test: %d ticks\r", diff);

  startTime = getClockTick();
  Send(2, (char *)shortMessage, 4, (char *)shortReply, 4);
  endTime = getClockTick();
  diff = startTime - endTime;
  if(diff < 0) diff += 508000;
  bwprintf(COM2, "4-byte(receive first) test: %d ticks\r", diff);
  Exit();
}

void firstTask() {
  clockInit();
  Create(0, producer_low);
  Create(1, consumer);
  Create(2, producer_high);
  Exit(); 
}
