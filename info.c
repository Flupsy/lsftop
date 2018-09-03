#include <string.h>
#include <limits.h>

#include "lsftop.h"


static void info_field_string(int y, int x, char *title, char *data)
{
	mvprintw(y, x, "%-30.30s %s", title, data);
}


static void info_field_float(int y, int x, char *title, float data)
{
	mvprintw(y, x, "%-30.30s %f", title, data);
}


static void info_field_int(int y, int x, char *title, int data)
{
  if(data==INT_MAX) {
    mvprintw(y, x, "%-30.30s None", title);
  }
  else {
    mvprintw(y, x, "%-30.30s %d", title, data);
  }
}


static void info_wait(void)
{
	timeout(-1);
	mvprintw(getmaxy(w)-1, 0, "Press any key to continue.");
	getch();
	timeout(interval);
}


void info_host(struct hostInfoEnt *host)
{
	int l=0;
	char status[40], reason[40];

	erase();
	mvprintw(l++, 0, "Host information for %s", host->host);
	move(l++, 0);
	hline('-', getmaxx(w));

	l++;

	*status=*reason='\0';

	if(host->hStatus & HOST_STAT_BUSY) {
		strcat(status, "Busy ");

		int busy=*(host->busySched) | *(host->busyStop);
		if(busy & HOST_BUSY_R15S)
			strcat(reason, "loadav(15s) ");
		if(busy & HOST_BUSY_R1M)
			strcat(reason, "loadav(1m) ");
		if(busy & HOST_BUSY_R15M)
			strcat(reason, "loadav(15m) ");
		if(busy & HOST_BUSY_UT)
			strcat(reason, "%CPU ");
		if(busy & HOST_BUSY_PG)
			strcat(reason, "PageRate ");
		if(busy & HOST_BUSY_IO)
			strcat(reason, "IORate ");
		if(busy & HOST_BUSY_LS)
			strcat(reason, "LoginSessions ");
		if(busy & HOST_BUSY_IT)
			strcat(reason, "IdleTime ");
		if(busy & HOST_BUSY_TMP)
			strcat(reason, "/tmp ");
		if(busy & HOST_BUSY_SWP)
			strcat(reason, "swap ");
		if(busy & HOST_BUSY_MEM)
			strcat(reason, "memory ");
	}

	if(host->hStatus & HOST_STAT_WIND)
		strcat(status, "WinClosed ");
	if(host->hStatus & HOST_STAT_DISABLED)
		strcat(status, "Disabled ");
	if(host->hStatus & HOST_STAT_LOCKED)
		strcat(status, "ExclLock ");
	if(host->hStatus & HOST_STAT_FULL)
		strcat(status, "Full ");
	if(host->hStatus & HOST_STAT_UNREACH)
		strcat(status, "Unreach ");
	if(host->hStatus & HOST_STAT_UNAVAIL)
		strcat(status, "!lim !sbatchd ");
	if(host->hStatus & HOST_STAT_UNLICENSED)
		strcat(status, "!licence ");
	if(host->hStatus & HOST_STAT_NO_LIM)
		strcat(status, "!lim ");

  if(!*status)
    strcpy(status, "OK");

	info_field_string(l++, 0, "Status", status);
	if(*reason)
		info_field_string(l++, 0, "Busy reason", reason);
  if(host->hCtrlMsg && *(host->hCtrlMsg))
    info_field_string(l++, 0, "Comment", host->hCtrlMsg);
	info_field_int(l++, 0, "Running jobs", host->numRUN);
	info_field_int(l++, 0, "Suspended jobs", host->numSSUSP);
	info_field_int(l++, 0, "User-suspended jobs", host->numUSUSP);
	info_field_int(l++, 0, "Number of reserved slots", host->numRESERVE);

	l++;

	info_field_float(l++, 0, "CPU factor", host->cpuFactor);
	info_field_int(l++, 0, "User job limit", host->userJobLimit);
	info_field_int(l++, 0, "Maximum jobs", host->maxJobs);
	info_field_int(l++, 0, "Migration threshold", host->mig);
	info_field_string(l++, 0, "Can checkpoint?", (host->attr & H_ATTR_CHKPNTABLE)? "Yes":"No");
	info_field_string(l++, 0, "Can checkpoint-copy?", (host->attr & H_ATTR_CHKPNT_COPY)? "Yes":"No");

	info_wait();
	erase();
}


void info_partition_list(void)
{
	struct hostPartInfoEnt *p;
	int partcount, i, j, l;

	if(!(p=lsb_hostpartinfo(NULL, &partcount))) {
		show_error(lsb_sysmsg());
		return;
	}

	timeout(-1);
	erase();

	mvprintw(0, 0, "List of host partitions");
	move(1, 0);
	hline('-', getmaxx(w));

	for(i=0; i<partcount; i++)
		mvprintw(i+3, 0, "%d.  %s", i+1, p[i].hostPart);

	mvprintw(i+2, 0, "Select a host partition and press return to view.");
	refresh();

	do {
		i=(int) getch()-'1';
	} while(i<1 && i>partcount);

	erase();
	l=0;
	mvprintw(l++, 0, "Host partition \"%s\"", p[i].hostPart);
	l++;

	info_field_string(l++, 0, "Hosts", p[i].hostList);
	l++;
	mvprintw(l++, 0, "User           Shares      Priority");
	move(l++, 0);
	hline('-', 80);

	for(j=0; j<p[i].numUsers; j++)
		mvprintw(l++, 0, "%-15.15s %d    %f",
			 p[i].users[j].user,
			 p[i].users[j].shares,
			 p[i].users[j].priority);

	info_wait();
	erase();
}
