#include "lsftop.h"
#include <stdlib.h>


static char *get_option(char *prompt)
{
	static char buf[16];

	timeout(-1);

	move(getmaxy(w)-2, 0);
	clrtobot();
	hline('-', getmaxx(w));
	move(getmaxy(w)-1, 0);
	printw(prompt);
	refresh();
	echo();
	wgetnstr(w, buf, sizeof(buf)-1);
	noecho();
	move(getmaxy(w)-2, 0);
	clrtobot();

	timeout(interval);

	return buf;
}


void set_interval(void)
{
	int new_interval;

	new_interval=atoi(get_option("Refresh interval (in seconds): "));
	if(new_interval<=0)
		show_error("Invalid refresh interval");
	else
		interval=new_interval*1000;
}


void enable_host(void)
{
  struct hostCtrlReq req;

	req.host=get_option("Host to enable: ");
  req.opCode=HOST_OPEN;

	if(*req.host)
		if(lsb_hostcontrol(&req)<0)
			show_error(lsb_sysmsg());
}


void disable_host(void)
{
  struct hostCtrlReq req;

	req.host=get_option("Host to disable: ");
  req.opCode=HOST_CLOSE;

	if(*req.host)
		if(lsb_hostcontrol(&req)<0)
			show_error(lsb_sysmsg());
}

void change_sort_order(void)
{
	enum sort_options old;

	if(format_option==FORMAT_SUMMARY)
		show_error("Sort order is fixed in summary mode");
	else {
		old=sort_option;
		sort_option++;
    if(sort_option==SORT_MAX)
      sort_option=SORT_NAME;

		if(resize_display()>0) {
			show_error("Terminal too small, and can't resize it");
			sort_option=old;
		} else
      options_changed=1;
	}
}


void get_host_info(void)
{
	char *hostname;
	struct hostInfoEnt *host;
	int i;

	hostname=get_option("Host to query: ");
	if(*hostname) {
		i=1;
		if(!(host=lsb_hostinfo(&hostname, &i)))
			show_error(lsb_sysmsg());
		else
			info_host(host);
	}
}
