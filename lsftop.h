#ifndef _LSFTOP_H_INCLUDED
#define	_LSFTOP_H_INCLUDED

#include <ncurses.h>
#include <netdb.h>
#include <lsf/lsf.h>
#include <lsf/lsbatch.h>

#define	LSFTOP_VERSION	"1.1"

extern enum sort_options
{
	SORT_NAME,
	SORT_GROUP,
  SORT_QUEUE,

	SORT_MAX
} sort_option;

extern enum form_options
{
	FORMAT_WIDE,
	FORMAT_SUMMARY,

	FORMAT_MAX
} format_option;

extern int debug_option;

extern WINDOW *w;
extern int interval, options_changed;
extern char *clustername, *mastername;
extern const char *build, *credits;


void resize_handler(int);
int resize_display(void);
void display_help(void);
void disable_host(void);
void enable_host(void);
void get_host_info(void);
void change_sort_order(void);
void update(void);
void set_interval(void);
void show_error(const char *);
void catch_fatal_signals(void);
void read_config_file(void);
void write_config_file(void);
void info_host(struct hostInfoEnt *);
void info_partition_list(void);

#endif
