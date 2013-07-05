#include <io.h>
#include <bwio.h>
#include <syscall.h>
#include <systemTasks.h>
#include <nameServer.h>
#include <values.h>
#include <ts7200.h>

#define BUFFERSIZE 128

struct IONode {
  int tid;
  int data;
  struct IONode *next;
};

struct IOMessagebuf{
  int type;
  int data[17];
};

struct IOMessage {
  int type;
  int data;
  int channel;
};

void handleNewInput(struct IONode **first, struct IONode *last, int *buffer, 
	int *bufHead, int bufTail, int input) {
  if(*first == NULL && *bufHead != ((bufTail + BUFFERSIZE - 1) % BUFFERSIZE)) {
    buffer[(*bufHead)++] = input;
    *bufHead %= BUFFERSIZE;
  }else if(*first != NULL) {
    Reply((*first)->tid, (char *)&input, sizeof(int));
    *first = (*first)->next;
  }
}

void handleNewInputTask(struct IONode **first, struct IONode **last, int *buffer, 
	int bufHead, int *bufTail, struct IONode *myNode) {
  if(bufHead != *bufTail) {
    Reply(myNode->tid, (char *)&buffer[(*bufTail)++], sizeof(int));
    *bufTail %= BUFFERSIZE;
  }else{
    if(*first == NULL) {
      *first = *last = myNode;
    }else{
      (*last)->next = myNode;
      *last = myNode;
    }
    myNode->next = NULL;
  }
}

void InputInit() {
  struct IOMessagebuf bufMsg;
  struct IOMessage *msg = (struct IOMessage *)&bufMsg;
  int reply, src;
  struct IONode nodes[100];
  struct IONode *first[2] = {NULL, NULL};
  struct IONode *last[2] = {NULL, NULL};
  int buffer1[BUFFERSIZE];
  int buffer2[BUFFERSIZE];
  int bufHead[2] = {0, 0};
  int bufTail[2] = {0, 0};

  int notifier1Tid = Create(7, notifier);
  int notifier2Tid = Create(7, bufferedNotifier);
  int eventType = TERMIN_EVENT;
  Send(notifier2Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  eventType = TRAIIN_EVENT;
  Send(notifier1Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  RegisterAs("Input Server");

  while(true) {
    Receive(&src, (char *)&bufMsg, sizeof(struct IOMessagebuf));
    switch(bufMsg.type) {
      case IONOTIFIER:
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	if(src == notifier1Tid){
	  handleNewInput(&first[0], last[0], buffer1, &bufHead[0], bufTail[0], (char)msg->data);
	}else if(src == notifier2Tid) {
	  int *buffer = bufMsg.data;
	  while(*buffer != (int)'\0') {
	    handleNewInput(&first[1], last[1], buffer2, &bufHead[1], bufTail[1], *buffer);
	    buffer++;
	  }
	}
	break;
      case IOINPUT:
        nodes[src & 0x7F].tid = src;
	if (msg->channel == 1){
	  handleNewInputTask(&first[0], &last[0], buffer1, bufHead[0], &bufTail[0], &nodes[src & 0x7F]);
	}else if(msg->channel == 2) {
	  handleNewInputTask(&first[1], &last[1], buffer2, bufHead[1], &bufTail[1], &nodes[src & 0x7F]);
	}
	break;
    }
  }
}

int handleNewOutput(int *buffer, int *bufHead, int *bufTail, int *available, int UARTBase, int cts) {
  if(*bufTail == *bufHead || !cts) {
    *available = true;
    return false;
  }else{
    int *UARTData = (int *)(UARTBase + UART_DATA_OFFSET);
    *UARTData = buffer[(*bufTail)++];
    *bufTail %= BUFFERSIZE;
    return true;
  }
}

int handleNewOutputTask(int *buffer, int *bufHead, int *bufTail, int *available, int UARTBase,
	int cts, int input) {
  int retVal = false;

  if(*available && cts) {
    *available = false;
    int *UARTData = (int *)(UARTBase + UART_DATA_OFFSET);
    *UARTData = input & 0xFF;
    retVal = true;
  }else{
    buffer[(*bufHead)++] = input & 0xFF;
    *bufHead %= BUFFERSIZE;
  }

  if((input & 0x80000000) == 0x80000000) {
    buffer[(*bufHead)++] = (input & 0xFF00) >> 8;
    *bufHead %= BUFFERSIZE;
  }

  return retVal;
}

void OutputInit() {
  struct IOMessage msg;
  int reply=0, src, ctsState=1; //assuming CTS defaults to on
  int buffer1[BUFFERSIZE];
  int buffer2[BUFFERSIZE];
  int bufHead[2] = {0, 0};
  int bufTail[2] = {0, 0};
  int available[2] = {false, false};
  
  int notifier1Tid = Create(7, notifier);
  int notifier2Tid = Create(7, notifier);
  int notifierCTSTid = Create(7, notifier);
  int eventType = TERMOUT_EVENT;
  Send(notifier2Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  eventType = TRAIOUT_EVENT;
  Send(notifier1Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int)); 
  eventType = TRAIMOD_EVENT;
  Send(notifierCTSTid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  RegisterAs("Output Server");

  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct IOMessage));
    switch(msg.type) {
      case IONOTIFIER:
	if (src == notifierCTSTid) {
	  Reply(notifierCTSTid, (char *)&reply, sizeof(int));
	  int *UART1_FLAGS = (int *)(UART1_BASE + UART_FLAG_OFFSET);
	  if(*UART1_FLAGS & CTS_MASK) {
	    if(bufHead[0] != bufTail[0] && available[0]) {
	      int *UART1_DATA = (int *)(UART1_BASE + UART_DATA_OFFSET);
	      *UART1_DATA = buffer1[bufTail[0]];
	      bufTail[0]++;
	      bufTail[0] %= BUFFERSIZE;
	    }else{
	      ctsState = 1;
	    }
 	  }
	}else if(src == notifier1Tid) {
	  if(handleNewOutput(buffer1, &bufHead[0], &bufTail[0], &available[0], UART1_BASE, ctsState)) {
	    Reply(notifier1Tid, (char *)&reply, sizeof(int));
	    ctsState = 0;
	  }
	}else if(src == notifier2Tid) {
	  if(handleNewOutput(buffer2, &bufHead[1], &bufTail[1], &available[1], UART2_BASE, 1)) {
	    Reply(notifier2Tid, (char *)&reply, sizeof(int));
	  } 
	}
	break;
      case IOOUTPUT:
	if (msg.channel == 1){
	  if(handleNewOutputTask(buffer1, &bufHead[0], &bufTail[0], &available[0], UART1_BASE, ctsState, msg.data)){
	    Reply(notifier1Tid, (char *)&reply, sizeof(int));
	    ctsState = 0;
	  } 
	}else if(msg.channel == 2) {
	  if(handleNewOutputTask(buffer2, &bufHead[1], &bufTail[1], &available[1], UART2_BASE, 1, msg.data)) {
	    Reply(notifier2Tid, (char *)&reply, sizeof(int));
	  }
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
    }
  }
}

int Getc(int channel) {
  struct IOMessage msg;
  msg.type = IOINPUT;
  msg.channel = channel;
  int c;
  Send(whoIs("Input Server"), (char *)&msg, sizeof(struct IOMessage), (char *)&c, sizeof(int));
  return c;
}

int Putc(int channel, char ch) {
  struct IOMessage msg;
  msg.type = IOOUTPUT;
  msg.channel = channel;
  msg.data = (int)ch;
  int reply;
  Send(whoIs("Output Server"), (char *)&msg, sizeof(struct IOMessage), (char *)&reply, sizeof(int));
  return 0;
}

int Putc2(int channel, char ch1, char ch2) {
  struct IOMessage msg;
  msg.type = IOOUTPUT;
  msg.channel = channel;
  msg.data = 0x80000000 | (ch2 << 8) | ch1;
  int reply;
  Send(whoIs("Output Server"), (char *)&msg, sizeof(struct IOMessage), (char *)&reply, sizeof(int));
  return 0;
}

//******************************************************************************************//

void printUnsignedInt(int channel, unsigned int n, int base) {
  unsigned int divisor = 1;

  while((n/divisor) >= base) divisor *= base;

  while(divisor != 0) {
    int digit = n/divisor;
    n %= divisor;
    divisor /= base;
    Putc(channel, (char)(digit + 48));
  }
}

void printInt(int channel, int n, int base) {
  if(n < 0) {
    n = -n;
    Putc(channel, '-');
  }
  printUnsignedInt(channel, n, base);
}

void printString(int channel, char *string) {
  while(*string != '\0') {
    Putc(channel, *string);
    string++;
  }
}

void formatString(char *format, va_list va) {
  char ch;
  while((ch = *(format++))) {
    if(ch != '%') {
      Putc(2, ch);
    }else{
      ch = *(format++);
      switch(ch) {
	case 'd':
	  printInt(2, va_arg(va, int), 10);
	  break;
	case 's':
	  printString(2, va_arg(va, char *));
	  break;
	case 'c':
	  Putc(2, va_arg(va, char));
	  break;
	case 'x':
	  printInt(2, va_arg(va, int), 16);
	  break;
      }
    }
  }
}

void printf(char *format, ...) {
  va_list va;

  va_start(va, format);
  formatString(format, va);
  va_end(va);
}

void outputEscape(char *escape) {
  while(*escape != '\0') {
    if(*escape == '[') {
      Putc(2, '\x1B');
      Putc(2, '[');
    }else{
      Putc(2, *escape);
    }
    escape++;
  }
}

void clearLine(int line) {
  outputEscape("[s");
  moveCursor(line, 1);
  outputEscape("[K[u");
}

void moveCursor(int line, int column) {
  outputEscape("[");
  printInt(2, line, 10);
  Putc(2, ';');
  printInt(2, column, 10);
  Putc(2, 'f');
}

void printAt(int line, int column, char *format, ...) {
  if(line < 0 || column < 0) {
    printf("printAt given negative line or column: %d:%d\r", line, column);
    return;
  }

  va_list va;

  requestDraw();
  outputEscape("[s");
  moveCursor(line, column);

  va_start(va, format);
  formatString(format, va);
  va_end(va);

  outputEscape("[u");
  finishedDrawing();
}

void printColored(int fColor, int bColor, char *format, ...) {
  va_list va;

  requestDraw();
  outputEscape("[");
  printInt(2, fColor+30, 10);
  Putc(2, ';');
  printInt(2, bColor+40, 10);
  Putc(2, 'm');

  va_start(va, format);
  formatString(format, va);
  va_end(va);

  outputEscape("[37;40m");
  finishedDrawing();
}

void sendTrainCommand(int command) {
  char second = command & 0xFF;
  char first = command >> 8;

  Putc(1, first);
  Putc(1, second);
}

/***********************************************************************************************/

struct DrawNode {
  int src;
  struct DrawNode *next;
};

void waitForStopMessage(struct DrawNode **first, struct DrawNode **last, struct DrawNode *nodes) {
  int src, reply, messageType;
  do{
    Receive(&src, (char *)&messageType, sizeof(int));
    if(messageType == DRAWSTOP) {
      reply = 0;
      Reply(src, (char *)&reply, sizeof(int));
    }else{
      struct DrawNode *myNode = &nodes[src & 0x7F];
      myNode->src = src;
      myNode->next = NULL;
      if(*first == NULL) {
        *first = *last = myNode;
      }else{
	(*last)->next = myNode;
	*last = myNode;
      }
    }
  }while(messageType != DRAWSTOP);
}

void DSInit() {
  struct DrawNode nodes[100];
  struct DrawNode *first = NULL;
  struct DrawNode *last = NULL;

  int src, reply, messageType;

  RegisterAs("Draw Server");

  while(true) {
    if(first == NULL) {
      Receive(&src, (char *)&messageType, sizeof(int));
      if(messageType == DRAWSTART) {
        reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	waitForStopMessage(&first, &last, nodes);
      }
    }else{	//first != NULL
      src = first->src;
      first = first->next;
      reply =0;
      Reply(src, (char *)&reply, sizeof(int));
      waitForStopMessage(&first, &last, nodes);
    }
  }
}

int requestDraw() {
  int messageType = DRAWSTART, reply;
  Send(whoIs("Draw Server"), (char *)&messageType, sizeof(int), (char *)&reply, sizeof(int));
  return 0;
}

int finishedDrawing() {
  int messageType = DRAWSTOP, reply;
  Send(whoIs("Draw Server"), (char *)&messageType, sizeof(int), (char *)&reply, sizeof(int));
  return 0;
}
