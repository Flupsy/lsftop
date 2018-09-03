#include "lsftop.h"


void display_help(void)
{
	int l=4;

	erase();

	mvprintw(2, 0, "The following keys will elicit some response:");
	mvprintw(l++, 0, "h    Display this screen (but you already knew that)");
	mvprintw(l++, 0, "q    Quit");
	mvprintw(l++, 0, "^L   Redraw screen");
	mvprintw(l++, 0, "h    Change refresh interval");
	mvprintw(l++, 0, "i    Display host information");
	mvprintw(l++, 0, "d    Disable a host");
	mvprintw(l++, 0, "e    Enable a host");
	mvprintw(l++, 0, "s    Toggle display of host groups");
	mvprintw(l++, 0, "p    Get information on host partitions");
	l++;
	mvprintw(l++, 0, "A * next to the hostname indicates that");
	mvprintw(l++, 0, "this host is acting as the LSF master.");
	l++;
	mvprintw(l++, 0, "Press any key to return to the main screen");

	timeout(-1);
	getch();
	timeout(interval);

	erase();
}
