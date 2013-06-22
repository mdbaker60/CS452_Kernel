#ifndef __IO_H__
#define __IO_H__

#define IONOTIFIER	0
#define IOINPUT		1
#define IOOUTPUT	2

#define DRAWSTART	0
#define DRAWSTOP	1

typedef char *va_list;

#define __va_argsiz(t)	\
		(((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap) ((void)0)

#define va_arg(ap, t)	\
		(((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

void InputInit();
void OutputInit();

int Getc(int channel);
int Putc(int channel, char ch);

void printString(int channel, char *string);
void printUnsignedInt(int channel, unsigned int n, int base);
void printInt(int channel, int n, int base);
void formatString(char *format, va_list va);
void printf(char *format, ...);
void outputEscape(char *escape);
void moveCursor(int line, int column);
void printAt(int line, int column, char *format, ...);
void sendTrainCommand(int command);

void DSInit();
int requestDraw();
int finishedDrawing();

#endif
