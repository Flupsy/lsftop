#include "lsftop.h"


void show_error(const char *str)
{
	move(getmaxy(w)-1, 0);
	clrtoeol();
	standout();
	printw("Error: %s", str);
	standend();

	refresh();
	sleep(3);
}
