#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

#include "lsftop.h"

#define	KEYWORD_VERSION	"version"
#define	KEYWORD_SORT	"sort"
#define	FILENAME	".lsftop"

static char *filename;


static int make_filename(void)
{
	struct passwd *pw;
  int len;

	if(!(pw=getpwuid(getuid()))) {
		fprintf(stderr, "I can't find you in the password file,"
				"so your .lsftop file will be ignored.");
		return 0;
	}

  len=strlen(FILENAME)+strlen(pw->pw_dir)+2;
	filename=(char *) malloc(len);
	snprintf(filename, len, "%s/"FILENAME, pw->pw_dir);

	return 1;
}


void read_config_file(void)
{
	FILE *fp;
	char *keyword, *arg, buf[128], *nl;
	int err=0, version_ok=0;

	if(!filename && !make_filename())
		return;

	if(!(fp=fopen(filename, "r")))
		return;

	while(!feof(fp)) {
		if(!fgets(buf, sizeof(buf), fp))
			break;

		if(!(keyword=strtok(buf, "	 =")))
			continue;

		if(!(arg=strtok(NULL, " 	=")))
			continue;

		if((nl=strchr(arg, '\n')))
			*nl='\0';

		if(strcmp(buf, KEYWORD_VERSION)==0) {
			version_ok=1;

			if(strcmp(arg, LSFTOP_VERSION)!=0) {
				fprintf(stderr, "Your %s file refers to version %s,\n"
						"but this is version %s - ignoring it.\n",
						FILENAME, arg, LSFTOP_VERSION);
				err++;
				break;
			}
		}
		else if(strcmp(buf, KEYWORD_SORT)==0) {
			sort_option=atoi(arg);
			if(sort_option>=SORT_MAX) {
				fprintf(stderr, "%s out of range in %s - ignoring\n",
					KEYWORD_SORT, FILENAME);
				err++;
			}
		}
		else {
			fprintf(stderr, "Unknown keyword \"%s\" in %s - ignoring\n",
				keyword, FILENAME);
			err++;
		}
	}

	fclose(fp);

	if(!version_ok) {
		fprintf(stderr, "No version number given in %s!\n", FILENAME);
		err++;
	}

	if(err)
		sleep(2);
}


void write_config_file(void)
{
	FILE *fp;

  if(!options_changed)
    return;

	if(!filename && !make_filename())
		return;

	if(!(fp=fopen(filename, "w")))
		return;

	fprintf(fp, "%s=%s\n", KEYWORD_VERSION, LSFTOP_VERSION);
	fprintf(fp, "%s=%d\n", KEYWORD_SORT, sort_option);

	fclose(fp);
}
