#include "lsftop.h"
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define	ATTR_HEADING		A_BOLD
#define	ATTR_HOST_ERROR		A_BLINK
#define	ATTR_HOST_CLOSED	A_DIM
#define	ATTR_QUEUE_QUIET	A_DIM
#define	ATTR_QUEUE_BUSY		0
#define	ATTR_QUEUE_FULL		A_BOLD

#define COLUMN_WIDTH    38


static int hostcount, groupcount, queuecount, partcount, max_columns;
static struct hostInfoEnt *hosts;
static struct queueInfoEnt *queues;
static struct groupInfoEnt *groups;
int row, col;

struct hostchecklist
{
	struct hostInfoEnt *host;
	int displayed;
};


int resize_display(void)
{
	int minsize;

	if(format_option==FORMAT_SUMMARY)
		minsize=queuecount+4;
	else if(sort_option==SORT_GROUP)
		minsize=((hostcount+6)/2)+6+queuecount;	/* XXX */
	else
		minsize=(hostcount/2)+6+queuecount;

	if(getmaxy(w)<minsize) {
		if(strcmp(termname(), "xterm")==0 || strcmp(termname(), "dtterm")==0)
			printf("\e[8;%d;80t", minsize);
		else
			return minsize;
	}

  max_columns=(int) (getmaxx(w)/COLUMN_WIDTH);

	return 0;
}


void resize_handler(int sig)
{
	endwin();
	w=initscr();
	cbreak();
	noecho();
	ungetch('\014');
	signal(SIGWINCH, resize_handler);
}


int compare_hosts(const void *p1, const void *p2)
{
	struct hostInfoEnt *h1=(struct hostInfoEnt *) p1;
	struct hostInfoEnt *h2=(struct hostInfoEnt *) p2;

	return strcmp(h1->host, h2->host);
}


int compare_groups(const void *p1, const void *p2)
{
	struct groupInfoEnt *g1=(struct groupInfoEnt *) p1;
	struct groupInfoEnt *g2=(struct groupInfoEnt *) p2;

	return strcmp(g1->group, g2->group);
}


int compare_queues(const void *p1, const void *p2)
{
	struct queueInfoEnt *q1=(struct queueInfoEnt *) p1;
	struct queueInfoEnt *q2=(struct queueInfoEnt *) p2;

	return strcmp(q1->queue, q2->queue);
}


int compare_hostnames(const void *p1, const void *p2)
{
	char *h1=*(char **) p1;
	char *h2=*(char **) p2;

	return strcmp(h2, h1);
}


int compare_hostchecks(const void *p1, const void *p2)
{
	struct hostchecklist *c1=(struct hostchecklist *) p1;
	struct hostchecklist *c2=(struct hostchecklist *) p2;

	return strcmp(c1->host->host, c2->host->host);
}


static void strip_trailing_slash(char *str)
{
  int len;

  len=strlen(str);
  if(str[len-1]=='/')
    str[len-1]='\0';
}


char *expand_hostlist(char *str)
{
  char *list, *name, *list_in_group, result[8192]="", *saveptr;
  struct hostInfoEnt *host, searchhost;
  struct groupInfoEnt *group, searchgroup;
  int i;

  list=strdup(str);

  for(i=0, name=strtok_r(list, " ", &saveptr); name; name=strtok_r(NULL, " ", &saveptr)) {
    strip_trailing_slash(name);

    searchhost.host=name;
    searchgroup.group=name;

    if((host=(struct hostInfoEnt *) bsearch(&searchhost, hosts, hostcount, sizeof(struct hostInfoEnt), compare_hosts))) {
      strcat(result, host->host);
      strcat(result, " ");
    }
    else if((group=(struct groupInfoEnt *) bsearch(&searchgroup, groups, groupcount, sizeof(struct groupInfoEnt), compare_groups))) {
      list_in_group=expand_hostlist(group->memberList);
      strcat(result, list_in_group);
      strcat(result, " ");
      free(list_in_group);
    }
  }
  
  free(list);

  return strdup(result);
}


static void sort_host_list(char *hostlist)
{
	char *ptrs[512];	/* Over 512 hosts and we're dead */
	char *str;
  char *saveptr;
	int i;

	str=strdup(hostlist);

	for(i=0, ptrs[i]=strtok_r(str, " ", &saveptr); ptrs[i]; ptrs[++i]=strtok_r(NULL, " ", &saveptr))
    strip_trailing_slash(ptrs[i]);

	qsort(ptrs, i, sizeof(char *), compare_hostnames);
	*hostlist='\0';
	while(i>0) {
		strcat(hostlist, ptrs[--i]);
		strcat(hostlist, " ");
	}

	free(str);
}


static char *host_flag(int flags, int *attr)
{
	static char *flag, buf[5];

	*attr=ATTR_HOST_ERROR;

  if(debug_option) {
    snprintf(buf, sizeof(buf), "%-4.4d", flags);
    flag=buf;
  }
	else if(LSB_HOST_UNLICENSED(flags))
		flag="UNLI";
	else if(LSB_HOST_UNREACH(flags) || LSB_HOST_UNAVAIL(flags))
		flag="DEAD";
	else if(flags & HOST_STAT_WIND)
		flag="WIND";
	else if(flags & HOST_STAT_NO_LIM)
		flag="!LIM";
	else {
		*attr=0;

		if(LSB_HOST_BUSY(flags))
			flag="BUSY";
		else if(LSB_HOST_OK(flags))
			flag=" OK ";
		else if(LSB_HOST_FULL(flags))
			flag="FULL";
		else if(LSB_HOST_CLOSED(flags)) {
			*attr=ATTR_HOST_CLOSED;
			flag="CLSD";
		}
		else {
			*attr=ATTR_HOST_ERROR;

			snprintf(buf, sizeof(buf), "%-4.4d", flags);
			flag=buf;
		}
	}

	return flag;
}


static char *queue_flag(int flags)
{
	static char *flag;

	if(!(flags & QUEUE_STAT_OPEN))
		flag="DSBL";
	else if(flags & QUEUE_STAT_RUNWIN_CLOSE)
		flag="WIND";
	else
		flag="OPEN";

	return flag;
}


static void print_host_info(int y, int x, struct hostInfoEnt *host)
{
	static char buf[128];
	char *flag, loadstr[4];
	int attr, load;

	switch(format_option) {
	case FORMAT_WIDE:
		snprintf(buf, sizeof(buf), "%s%c", host->host, strcmp(host->host, mastername)==0? '*':'\0');

		flag=host_flag(host->hStatus, &attr);
		attron(attr);

		load=(int) (host->load[UT]*100);
		if(load>100)
			strcpy(loadstr, "n/a");
		else
			snprintf(loadstr, sizeof(loadstr), "%3d", load);

		mvprintw(y, x, "%-9.9s %s  %3d %3d %3d %3d %s",
				buf,
				flag,
				host->numRUN,
				host->numSSUSP+host->numUSUSP,
				host->numRESERVE,
				host->maxJobs-host->numJobs,
				loadstr);

		attroff(attr);

		break;

	case FORMAT_SUMMARY:
		/* do nothing - for now */
		break;

  case FORMAT_MAX:
  default:
    break;
	}
}


static int new_column(void)
{
  if(++col==max_columns)
    return 0;

  return col;
}


void update(void)
{
	int i, header, total_slots=0, total_slots_used=0;
	struct hostchecklist *hostchecklist=NULL, *hostcheck, keyhost;
	struct groupInfoEnt keygroup;
	struct hostPartInfoEnt *parts;
	char buf[128];
	time_t now;

	hostcount=queuecount=groupcount=partcount=0;

	if(!(hosts=lsb_hostinfo(NULL, &hostcount))) {
		show_error(lsb_sysmsg());
		return;
	}

  for(i=0; i<hostcount; i++)
    strip_trailing_slash(hosts[i].host);

	qsort(hosts, hostcount, sizeof(struct hostInfoEnt), compare_hosts);

	/*
	 * Make a copy of the host list so that we can cross off
	 * hosts as we display them (SORT_GROUP mode only).
	 */

	if(sort_option==SORT_GROUP || sort_option==SORT_QUEUE) {
		hostchecklist=(struct hostchecklist *) calloc(hostcount, sizeof(struct hostchecklist));
		for(i=0; i<hostcount; i++) {
			hostchecklist[i].host=&hosts[i];
			hostchecklist[i].displayed=0;
		}
	}

	if(!(queues=lsb_queueinfo(NULL, &queuecount, NULL, NULL, 0))) {
		show_error(lsb_sysmsg());
		return;
	}
	qsort(queues, queuecount, sizeof(struct queueInfoEnt), compare_queues);

  for(i=0; i<queuecount; i++)
    strip_trailing_slash(queues[i].queue);

  /*
   * We need both host group and partition info, so that we
   * can ignore partitions here (they also get reported as
   * host groups (bug?)).
   */

  if(!(groups=lsb_hostgrpinfo(NULL, &groupcount, GRP_ALL))) {
    show_error(lsb_sysmsg());
    return;
  }

  for(i=0; i<groupcount; i++)
    strip_trailing_slash(groups[i].group);

  qsort(groups, groupcount, sizeof(struct groupInfoEnt), compare_groups);

  if(!(parts=lsb_hostpartinfo(NULL, &partcount))) {
    if(lsberrno!=LSBE_NO_HPART) {
      show_error(lsb_sysmsg());
      return;
    }
  }

	if((i=resize_display())>0) {
		endwin();
		fprintf(stderr, "Terminal must be at least 80x%d.\n"
			"Try using the -s switch for summary mode.\n",
			i);
		exit(1);
	}

	attron(ATTR_HEADING);
	switch(format_option) {
	case FORMAT_WIDE:
    for(i=0; i<max_columns; i++) {
      mvprintw(0, i*COLUMN_WIDTH, "HOSTNAME  STAT  RUN SSP RSV AVA %%UT");
      move(0, ((i+1)*COLUMN_WIDTH)-2);
      vline('|', getmaxy(w)-queuecount-3);
    }
		break;

	case FORMAT_SUMMARY:
		mvprintw(0, 0, "Cluster summary information (-s option used)");
		break;

  case FORMAT_MAX:
  default:
    break;
	}

	move(1, 0);
	hline('-', getmaxx(w));
	attroff(ATTR_HEADING);

	switch(sort_option) {
	case SORT_NAME:
		for(i=0, row=2, col=0; i<hostcount; i++) {
			if(row>getmaxy(w)-queuecount-6) {
        if(!new_column()) {
          //mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
          //hline(' ', 1000);
          goto sort_name_end;
        }
        row=2;
      }

			print_host_info(row++, col*COLUMN_WIDTH, &hosts[i]);
		}

    sort_name_end:
		break;

  case SORT_QUEUE:
    switch(format_option) {
    case FORMAT_WIDE:
      for(i=0, row=2, col=0; i<queuecount; i++) {
        char *str, *saveptr1, *saveptr2;
        struct hostInfoEnt *host, searchhost;

        if(row>getmaxy(w)-queuecount-5) {
          if(!new_column()) {
            mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
            hline(' ', 1000);
            goto sort_queue_end;
          }
          row=2;
        }
        attron(ATTR_HEADING);
        mvprintw(row++, col*COLUMN_WIDTH, "-- %s --", queues[i].queue);
        attroff(ATTR_HEADING);

        for(str=strtok_r(queues[i].hostList, " ", &saveptr1); str; str=strtok_r(NULL, " ", &saveptr1)) {
          /*
           * Despite what the documentation says, hostList can contain
           * both hostames and hostgroup names, optionally suffixed
           * with priority numbers
           */

          char *hostlist, *thishost, *prio;

          if((prio=strchr(str, '+')))
            *prio='\0';

          hostlist=expand_hostlist(str);
          sort_host_list(hostlist);

          for(thishost=strtok_r(hostlist, " ", &saveptr2); thishost; thishost=strtok_r(NULL, " ", &saveptr2)) {
            searchhost.host=thishost;

            if((host=(struct hostInfoEnt *) bsearch(&searchhost, hosts, hostcount, sizeof(struct hostInfoEnt), compare_hosts)))
              print_host_info(row++, col*COLUMN_WIDTH, host);
            else
              mvprintw(row++, col*COLUMN_WIDTH, "%s: no info available (host)", thishost);

            if(row>getmaxy(w)-queuecount-4) {
              if(!new_column()) {
                mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
                hline(' ', 1000);
                free(hostlist);
                goto sort_queue_end;
              }
              row=2;
            }
          }

          free(hostlist);
				}
      }

      sort_queue_end:
      break;

    case FORMAT_SUMMARY:
    case FORMAT_MAX:
    default:
      break;
    }

    break;

	case SORT_GROUP:
		keyhost.host=(struct hostInfoEnt *) malloc(sizeof(struct hostInfoEnt));

		for(i=0; i<partcount; i++) {
			struct groupInfoEnt *g;

			keygroup.group=parts[i].hostPart;
			if((g=(struct groupInfoEnt *) bsearch(&keygroup, groups, groupcount, sizeof(struct groupInfoEnt), compare_groups)))
				*(g->group)='\0';
		}

		switch(format_option) {
		case FORMAT_WIDE:
			for(i=0, row=2, col=0; i<groupcount; i++) {
				char *hostlist, *str, *saveptr;

				if(!*(groups[i].group))
					continue;

				if(row>getmaxy(w)-queuecount-5) {
          if(!new_column()) {
            mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
            hline(' ', 1000);
            goto sort_group_end;
          }
					row=2;
				}
				attron(ATTR_HEADING);
				mvprintw(row++, col*COLUMN_WIDTH, "-- %s --", groups[i].group);
				attroff(ATTR_HEADING);

        hostlist=expand_hostlist(groups[i].memberList);
				sort_host_list(hostlist);
				for(str=strtok_r(hostlist, " ", &saveptr); str; str=strtok_r(NULL, " ", &saveptr)) {
					keyhost.host->host=str;
					if(!(hostcheck=(struct hostchecklist *) bsearch(&keyhost, hostchecklist, hostcount, sizeof(struct hostchecklist), compare_hostchecks)))
						mvprintw(row++, col*COLUMN_WIDTH, "%s: no info available", str);
					else {
						print_host_info(row++, col*COLUMN_WIDTH, hostcheck->host);
						hostcheck->displayed=1;
					}

					if(row>getmaxy(w)-queuecount-4) {
            if(!new_column()) {
              mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
              hline(' ', 1000);
              goto sort_group_end;
            }
            row=2;
					}
				}
			}

			for(i=0, header=0; i<hostcount; i++) {
				if(!hostchecklist[i].displayed) {
					if(!header) {
						attron(ATTR_HEADING);
						mvprintw(row++, col*COLUMN_WIDTH, "-- (no group) --");
						attroff(ATTR_HEADING);

						header=1;
					}

					print_host_info(row++, col*COLUMN_WIDTH, hostchecklist[i].host);

					if(row>getmaxy(w)-queuecount-4) {
            if(!new_column()) {
              mvprintw(row-1, (col-1)*COLUMN_WIDTH, "... more (window too small) ...");
              hline(' ', 1000);
              goto sort_group_end;
            }
						row=2;
					}
				}
			}

			//if(hostcheck)
			//	free(hostcheck->host);

      sort_group_end:
			break;

		case FORMAT_SUMMARY:
			mvprintw(3, 0, "Patience is a virtue.");
			mvprintw(4, 0, "(i.e. not implemented yet)");
			break;

    case FORMAT_MAX:
    default:
      break;
		}

		free(keyhost.host);

		break;

  case SORT_MAX:
  default:
    break;
	}

	attron(ATTR_HEADING);
	mvprintw(getmaxy(w)-queuecount-2, 0, "QUEUE            PRI  RUN  PEND SUSP RSV  STAT");
	move(getmaxy(w)-queuecount-1, 0);
	hline('-', 47);
	attroff(ATTR_HEADING);

	for(i=0; i<queuecount; i++) {
		int attr;

		if(queues[i].numRUN+queues[i].numPEND+queues[i].numSSUSP+queues[i].numUSUSP==0)
			attr=ATTR_QUEUE_QUIET;
		else if(queues[i].numPEND)
			attr=ATTR_QUEUE_FULL;
		else
			attr=ATTR_QUEUE_BUSY;

		attron(attr);

		switch(format_option) {
		case FORMAT_WIDE:
		case FORMAT_SUMMARY:
			mvprintw(getmaxy(w)-(queuecount-i), 0, "%-15.15s  %3d  %3d  %3d  %3d  %3d  %s  ",
				 queues[i].queue,
				 queues[i].priority,
				 queues[i].numRUN,
				 queues[i].numPEND,
				 queues[i].numSSUSP+queues[i].numUSUSP,
				 queues[i].numRESERVE,
				 queue_flag(queues[i].qStatus));

    case FORMAT_MAX:
    default:
      break;
		}

		attroff(attr);
	}

  for(i=0; i<hostcount; i++) {
    if(LSB_HOST_BUSY(hosts[i].hStatus) ||
       LSB_HOST_OK(hosts[i].hStatus) ||
       LSB_HOST_FULL(hosts[i].hStatus)) {
      total_slots+=hosts[i].maxJobs;
      total_slots_used+=hosts[i].numJobs;
    }
  }

  mvprintw(getmaxy(w)-queuecount-2, 55, "%d/%d (%d%%) slots in use",
           total_slots_used, total_slots, (total_slots_used*100)/total_slots);

	snprintf(buf, sizeof(buf), "lsftop version %s", LSFTOP_VERSION);
	mvprintw(getmaxy(w)-7, getmaxx(w)-strlen(buf), "lsftop version %s", LSFTOP_VERSION);
	mvprintw(getmaxy(w)-6, getmaxx(w)-strlen(credits), credits);
	mvprintw(getmaxy(w)-5, getmaxx(w)-strlen(build), build);
	snprintf(buf, sizeof(buf), "Cluster: %s", clustername);
	mvprintw(getmaxy(w)-3, getmaxx(w)-strlen(buf), buf);

	time(&now);
	strftime(buf, sizeof(buf)-1, "%d/%m/%Y %H:%M:%S", localtime(&now));
	mvprintw(getmaxy(w)-1, getmaxx(w)-strlen(buf), buf);

	move(0, getmaxx(w));
	refresh();

	if(hostchecklist)
		free(hostchecklist);
}


