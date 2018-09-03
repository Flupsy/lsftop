#include <stdio.h>
#include <signal.h>

#include "lsftop.h"


void fatal_signal_handler(int sig)
{
	endwin();
	printf("\n\nOh no, we just got a %s (%d)!\n\nPlease email ian@chard.org\n"
	       "and tell him how you broke his program.\n\n", _sys_siglist[sig], sig);
	fflush(stdout);

	_exit(0);
}


void catch_fatal_signals(void)
{
	signal(SIGQUIT, fatal_signal_handler);
	signal(SIGILL, fatal_signal_handler);
	signal(SIGTRAP, fatal_signal_handler);
	signal(SIGABRT, fatal_signal_handler);
	signal(SIGFPE, fatal_signal_handler);
	signal(SIGBUS, fatal_signal_handler);
	signal(SIGSEGV, fatal_signal_handler);
	signal(SIGSYS, fatal_signal_handler);
}
