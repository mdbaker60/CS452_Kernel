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

  int notifier2Tid = Create(7, notifier);
  int eventType = TERMIN_EVENT;
  Send(notifier2Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  //RegisterAs("Input Server");

  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct IOMessage));
    switch(msg.type) {
      case IONOTIFIER:
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	if(src == notifier2Tid) {
	  handleNewInput(&first[1], last[1], buffer2, &bufHead[1], bufTail[1], msg.data);
	}
	break;
      case IOINPUT:
        nodes[src & 0x7F].tid = src;
	if(msg.channel == 2) {
	  handleNewInputTask(&first[1], &last[1], buffer2, bufHead[1], &bufTail[1], &nodes[src & 0x7F]);
	}
	break;
    }
  }
}

int handleNewOutput(struct IONode **first, struct IONode *last, int *available, int UARTBase) {
  if(*first == NULL) {
    *available = true;
    return false;
  }else{
    int *UARTData = (int *)(UARTBase + UART_DATA_OFFSET);
    *UARTData = (*first)->data;
    int reply = 0;
    Reply((*first)->tid, (char *)&reply, sizeof(int));
    *first = (*first)->next;
    return true;
  }
}

int handleNewOutputTask(struct IONode **first, struct IONode **last, int *available, int UARTBase,
	struct IONode *myNode) {
  if(*available) {
    *available = false;
    int *UARTControl = (int *)(UARTBase + UART_CTLR_OFFSET);
    int *UARTData = (int *)(UARTBase + UART_DATA_OFFSET);
    *UARTData = myNode->data;
    *UARTControl |= TIEN_MASK;
    int reply = 0;
    Reply(myNode->tid, (char *)&reply, sizeof(int));
    return true;
  }else{
    if(*first == NULL) {
      *first = *last = myNode;
    }else{
      (*last)->next = myNode;
      *last = myNode;
    }
    myNode->next = NULL;
    return false;
  }
}

void OutputInit() {
  struct IOMessage msg;
  int reply=0, src;
  struct IONode nodes[100];
  struct IONode *first[2] = {NULL, NULL};
  struct IONode *last[2] = {NULL, NULL};
  int available[2] = {false, false};

  int notifier2Tid = Create(7, notifier);
  int eventType = TERMOUT_EVENT;
  Send(notifier2Tid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  //RegisterAs("Output Server");

  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct IOMessage));
    switch(msg.type) {
      case IONOTIFIER:
	if(src == notifier2Tid) {
	  if(handleNewOutput(&first[1], last[1], &available[1], UART2_BASE)) {
	    Reply(notifier2Tid, (char *)&reply, sizeof(int));
	  } 
	}
	break;
      case IOOUTPUT:
	nodes[src & 0x7F].tid = src;
	nodes[src & 0x7F].data = msg.data;
	if(msg.channel == 2) {
	  if(handleNewOutputTask(&first[1], &last[1], &available[1], UART2_BASE, &nodes[src & 0x7F])) {
	    Reply(notifier2Tid, (char *)&reply, sizeof(int));
	  }
	}
	break;
    }
  }
}

int Getc(int channel) {
  struct IOMessage msg;
  msg.type = IOINPUT;
  msg.channel = channel;
  int c;
  //Send(whoIs("Input Server"), (char *)&msg, sizeof(struct IOMessage), (char *)&c, sizeof(int));
  Send(4, (char*)&msg, sizeof(struct IOMessage), (char *)&c, sizeof(int));
  return c;
}

int Putc(int channel, char ch) {
  struct IOMessage msg;
  msg.type = IOOUTPUT;
  msg.channel = channel;
  msg.data = (int)ch;
  int reply;
  //Send(whoIs("Output Server"), (char *)&msg, sizeof(struct IOMessage), (char *)&reply, sizeof(int));
  Send(6, (char *)&msg, sizeof(struct IOMessage), (char *)&reply, sizeof(int));
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
