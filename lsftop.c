/*
 * lsftop:  display LSF cluster status
 *
 * Ian Chard  31/5/02
 */

#include "lsftop.h"
#include <locale.h>
#include <signal.h>


WINDOW *w;
enum sort_options sort_option=SORT_NAME;
enum form_options format_option=FORMAT_WIDE;
int debug_option=0;
int interval=10000;
int options_changed=0;
char *clustername, *mastername;

const char *build="Built "__DATE__" "__TIME__;
const char *credits="by Ian Chard";


int main(int argc, char *argv[])
{
	char c;
	int err=0;

	setlocale(LC_ALL, "");

	catch_fatal_signals();
	read_config_file();

	while((c=getopt(argc, argv, "vsd"))!=EOF)
		switch(c) {
		case 'v':
			fprintf(stderr, "lsftop version %s\n%s\n%s\n", LSFTOP_VERSION, build, credits);
			return 0;
		case 's':
			format_option=FORMAT_SUMMARY;
			sort_option=SORT_GROUP;
			break;
    case 'd':
      debug_option=1;
      break;
		default:
			err++;
		}

	if(err) {
		fprintf(stderr, "usage: %s [-vs]\n", argv[0]);
		return 0;
	}

	if(lsb_init(argv[0])<0) {
		lsb_perror("lsb_init");
		return 1;
	}

	if(!(mastername=ls_getmastername())) {
		ls_perror("ls_getmastername");
		return 1;
	}

	if(!(clustername=ls_getclustername())) {
		ls_perror("ls_getclustername");
		return 1;
	}

	w=initscr();
	cbreak();
	noecho();
	timeout(interval);

	signal(SIGWINCH, resize_handler);

	for(;;) {
		update();
		switch(getch()) {
		case ERR:	/* timeout - no key pressed */
			break;
		case 'h':
		case '?':
			display_help();
			break;
		case 'r':
			set_interval();
			break;
		case 'i':
			get_host_info();
			break;
		case 'd':
			disable_host();
			break;
		case 'e':
			enable_host();
			break;
		case 's':
			change_sort_order();
			clear();
			break;
		case 'p':
			info_partition_list();
			break;
		case '\014':
			clear();
			break;
		case 'q':
			goto sodoff;
		}
	}

sodoff:
	endwin();

	write_config_file();

	return 0;
}
