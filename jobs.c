#include <stdio.h>
#include <stdlib.h>
#include "lsftop.h"

char **getIndexList(int *listsize)
{
    struct lsInfo *lsInfo = (struct lsInfo *) malloc (sizeof
    (struct lsInfo));
    static char *nameList[268];
    static int first = 1;
    int i;
    if (first) {
        /* only need to do so when called for the first time */
        lsInfo = ls_info();
        if (lsInfo == NULL)
            return (NULL);
        first = 0;
    }
    if (listsize != NULL)
        *listsize = lsInfo->numIndx;
    for (i=0; i<lsInfo->numIndx; i++)
        nameList[i] = lsInfo->resTable[i].name;
    return (nameList);
}

int main(void)
{
  struct jobInfoEnt *info;
  struct jobInfoHead *jobs;
  struct loadIndexLog indices;
  char *reason;
  int i;

  lsb_init("jobs");
  jobs=lsb_openjobinfo_a(0, NULL, "all", NULL, NULL, PEND_JOB);

  for(i=0; i<jobs->numJobs; i++) {
    info=lsb_readjobinfo(NULL);
    printf("%d: status=%d, reasons=%d, subreasons=%d\n", LSB_ARRAY_JOBID(info->jobId), info->status, info->numReasons, info->subreasons);
    if(info->status==JOB_STAT_PEND) {
      indices.name=getIndexList(&indices.nIdx);
      reason=lsb_pendreason(info->numReasons, info->reasonTb, jobs, &indices, 0);
      printf("\t%s\n", reason);
      reason=lsb_suspreason(info->reasons, info->subreasons, &indices);
      printf("\t%s\n", reason);
    }
  }

  lsb_closejobinfo();

  return 0;
}
