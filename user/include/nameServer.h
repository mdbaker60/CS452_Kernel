#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#define NSREGISTER	0
#define NSWHOIS		1
#define NSSHUTDOWN	2

void NSInit();
int whoIs(char* task);
int RegisterAs(char *task);

#endif
