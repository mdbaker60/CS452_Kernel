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
    Reply(myNode->tid, (char *)buffer[(*bufTail)++], sizeof(int));
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
  struct IOMessage msg;
  int reply, src;
  struct IONode nodes[100];
  struct IONode *first[2] = {NULL, NULL};
  struct IONode *last[2] = {NULL, NULL};
  int buffer1[BUFFERSIZE];
  int buffer2[BUFFERSIZE];
  int bufHead[2] = {0, 0};
  int bufTail[2] = {0, 0};

  int notifier1Tid = Create(7, notifier);
  int notifier2Tid = Create(7, notifier);
  int eventType = TERMIN_EVENT;
  Send(notifier2Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  eventType = TRAIIN_EVENT;
  Send(notifier1Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  RegisterAs("Input Server");

  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct IOMessage));
    switch(msg.type) {
      case IONOTIFIER:
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	if(src == notifier1Tid){
	  handleNewInput(&first[0], last[0], buffer1, &bufHead[0], bufTail[0], msg.data);
	}else if(src == notifier2Tid) {
	  handleNewInput(&first[1], last[1], buffer2, &bufHead[1], bufTail[1], msg.data);
	}
	break;
      case IOINPUT:
        nodes[src & 0x7F].tid = src;
	if (msg.channel == 1){
	  handleNewInputTask(&first[0], &last[0], buffer1, bufHead[0], &bufTail[0], &nodes[src & 0x7F]);
	}else if(msg.channel == 2) {
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
  if(*available && cts) {
    *available = false;
    int *UARTData = (int *)(UARTBase + UART_DATA_OFFSET);
    *UARTData = input;
    return true;
  }else{
    buffer[(*bufHead)++] = input;
    *bufHead %= BUFFERSIZE;
    return false;
  }
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
	    ctsState = 1;
	    if(bufHead[0] != bufTail[0] && available[0]) {
	      int *UART1_DATA = (int *)(UART1_BASE + UART_DATA_OFFSET);
	      *UART1_DATA = buffer1[bufTail[0]];
	      bufTail[0]++;
	      bufTail[0] %= BUFFERSIZE;
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

void sendTrainCommand(int command) {
  char second = command & 0xFF;
  char first = command >> 8;

  Putc(1, first);
  Putc(1, second);
}
