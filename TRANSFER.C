/*********************************************************************************************************************************
**
**     TRANSFER.EXE    -       Data transfer test for OS/2
**     
**     TRANSFER <drive letter> [<block_size in K.> <number of blocks to transfer>]
**
**     Reads <block_size> <blocks> from <drive letter> and calculates a transfer rate.
**
**     Reads are done using DosRead API.
**
**********************************************************************************************************************************/
void usage(void);

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>

#include    "video.h"

#define     INCL_DOS
#include    <os2.h>

#define SECONDS_PER_HOUR   3600L
#define SECONDS_PER_MINUTE 60L

extern  int main(int argc,char * *argv);
extern  int transfer_test(int handle,int blocks,long block_size,char far *buf,double *rate);
extern  int get_mem(long size,char far * *buf);
extern  void results(double rate,int blocks,long block_size);
extern  int open_d_disk(int disk,int *handle);
extern  int close_d_disk(int handle);
extern  void usage(void );
extern  void diff_time(struct _DATETIME far *stop,struct _DATETIME far *start,struct _DATETIME far *diff);
extern  unsigned long get_hundreds(struct _DATETIME far *time);

int main(int argc, char **argv)
{
       int     drive;                                               /* drive letter to test, 'C', 'D' etc. */
       int     blocks = 100;                                        /* number of blocks to read */
       long    block_size = 64L;                                    /* size (in K) of each block */
       int     handle;                                              /* handle to drive for DosRead() */
       char far *buf;                                               /* buffer to read into */
       int     index = 1;                                           /* index into command line args */
       double  rate;                                                /* rate in K/second */
       PIDINFO id;                                                  /* program id information */
/******************************************************************************************************************************/
                                                                    /* say hello */
       printf("TRANSFER (OS/2 data transfer test) version 1.1\n");

       if (argc < 2)                                                /* check args */
               {
               usage();
               return 1;
               }


       DosGetPID(&id);                                              /* get id's */
       DosSetPrty(2, 3, 31, id.tid);                                /* and set priority to highest level, time critical, this thread */

       strupr(argv[index]);                                         /* convert first arg to uppercase */
       drive = *argv[index++];                                      /* get drive */
       if (argv[index] != NULL)                                     /* if second arg */
               block_size = atoi(argv[index++]);                    /* must be block size */
       if (argv[index] != NULL)                                     /* if third arg */
               blocks = atoi(argv[index++]);                        /* must be number of blocks */

       if (block_size > 64)                                         /* if more than one segment */
               {                                                    /* exit */
               usage();
               return 1;
               }

       if (get_mem(block_size, &buf))                               /* allocate memory from OS/2 */
               {
               fprintf(stderr, "Memory allocation error, exiting...\n");
               return 1;
               }

       if (open_d_disk(drive -64, &handle))                         /* get handle to whole disk */
               {
               fprintf(stderr, "Can't get access to drive %c:, exiting...\n", drive);
               return 1;
               }

       if (transfer_test(handle, blocks, block_size, buf, &rate))    /* do transfer test */
               {
               fprintf(stderr, "Read error during test, exiting...\n");
               return 1;
               }

       if (close_d_disk(handle))                                    /* releast handle to disk */
               {
               fprintf(stderr, "Can't release access to drive %c:, exiting...\n", drive);
               return 1;
               }

       results(rate, blocks, block_size);                           /* display results */
       return 0;
}

/*********************************************************************************************************************************
**
**
** TRANSFER_TEST       -       Does actual test.
**
**
**********************************************************************************************************************************/
int transfer_test(int handle, int blocks, long block_size, char far *buf, double *rate)
{
       register int i;                                              /* loop */
       DATETIME start, end, diff;                                   /* times */
       unsigned bytes;                                              /* number of bytes actually read */
       double hundredths;                                           /* hundredths of a second test took */
       unsigned long newptr;                                        /* file pointer position after DosChgFilePtr() */
/******************************************************************************************************************************/

       block_size *= 1024;                                          /* make block size bytes */

       if (block_size == 0x10000)                                   /* if exactly 64K make 1 byte less */
               block_size = 0xffff;

       DosChgFilePtr(handle, 0L, 0, &newptr);                       /* get to start before timing starts */
       if (DosRead(handle, buf, (USHORT)block_size, (PUSHORT)&bytes))    /* read to force a seek */
               return 1;

       DosGetDateTime(&start);                                      /* get start time */

       for (i=0; i<blocks; i++)                                     /* loop thru blocks */
               {
               DosChgFilePtr(handle, 0L, 0, &newptr);               /* seek */
               if (DosRead(handle, buf, (USHORT)block_size, (PUSHORT)&bytes))    /* read */
                       return 1;
               }
       DosGetDateTime(&end);                                        /* all done, get time */

       diff_time(&end, &start, &diff);                              /* get difference in times */
       hundredths = (double)get_hundreds(&diff);                    /* how many hundredths? */
                                                                    /* calculate rate */
       *rate = (((double)blocks * (double)block_size) / 1024.0) / (hundredths / 100.0);
       return 0;
}

/*********************************************************************************************************************************
**
** GET_MEM     -       Allocate a selector from OS/2
**
**********************************************************************************************************************************/
int get_mem(long size, char far **buf)
{
       unsigned *sel;                                               /* selector */
/******************************************************************************************************************************/

       if (size == 64)                                              /* if want 64K, */
               size = 0;                                            /* set to 0 */
       else
               size *= 1024;                                        /* convert to bytes */
       
       if(DosAllocSeg((USHORT)size, (PUSHORT)sel, 0))               /* get segment */
               return 1;

       *buf = MAKEP(*sel, 0);                                       /* convert to pointer */
       return 0;
}


/*********************************************************************************************************************************
**
** RESULTS     -       displays results to user
**
**********************************************************************************************************************************/
void results(double rate, int blocks, long block_size)
{
/******************************************************************************************************************************/

       printf("\nTransfer rate is: %1.1lfK per second,\n"
              "using %d blocks with a block size of %ldK.\n\n", rate, blocks, block_size);

       
       wrtnum(blocks * block_size * 1024, 0, 0, BOLD | NORMAL, 1);
       printf(" bytes were transfered during the test.\n");
}


/*********************************************************************************************************************************
** int   open_d_disk(int   disk, int *handle)
**       int   disk;                drive to open, 1 = A:, 2 = B:, etc.
**       int   *handle;             handle for disk, can be used like a file handle
**
**    Returns (0) if success,
**            (Error code) if error.
**
**********************************************************************************************************************************/

int   open_d_disk(int disk, int *handle)
{
   char  inbuf[3];                                                             /* input buffer, filename */
   int   action;                                                               /* receives action code */
   unsigned int   mode = 0x12 | 0xE000;                                        /* mode = DASD open, unbuffered, Fail on errors */
                                                                               /* children inherit, no sharing, read/write */
                                                                               /* 1110000000010010 */
/******************************************************************************************************************************/

   inbuf[0] = (char) disk + 64;
   inbuf[1] = ':';
   inbuf[2] = '\0';

   return(DosOpen(inbuf, (PHFILE)handle, (PUSHORT)&action, 0L, 0, 1, mode, 0L));
}


/*********************************************************************************************************************************
**
** CLOSE_D_DISK        -       releases handle to disk for others
**
**********************************************************************************************************************************/
int   close_d_disk(int handle)
{
   return(DosClose(handle));                                        /* close file handle */
}

/*********************************************************************************************************************************
**
** USAGE       -       Displays syntax of program
**
**********************************************************************************************************************************/
void usage(void)
{
       printf("Usage: Transfer <drive letter> [<block size> <number of blocks>]\n\n"
              "       Where    <drive letter>          - OS/2 drive letter, 'C:' etc.\n"
              "                <block size>            - Size of each transfer in Kilobytes\n"
              "                                          default is 64, range is 1-64\n"
              "                <number of blocks>      - Number of blocks to transfer\n"
              "                                          default is 100 blocks\n");
}



void diff_time(PDATETIME stop, PDATETIME start, PDATETIME diff)

/*  Purpose:   Compute the time difference between stop and start
**  Receives:  stop, start, diff: pointers to structures containing:
**                hours      -- caller's hours var
**                minutes    -- caller's minutes var
**                seconds    -- caller's seconds var
**                hundredths -- caller's hundredths var
**  Returns:   nothing
**  Remarks:   pseudo math: diff = ( stop - start )
**             This routine will compute the difference up to 23:59:59.99
*/

{
/* convert start/stop into seconds, subtract, convert stop to HH:MM:SS.ss */
       int diff_hundreds;

       long stop_seconds = stop->hours * SECONDS_PER_HOUR
       + stop->minutes * SECONDS_PER_MINUTE
       + stop->seconds;                                             /* take care of hundredths later */

       long start_seconds = start->hours * SECONDS_PER_HOUR
       + start->minutes * SECONDS_PER_MINUTE
       + start->seconds;                                            /* take care of hundredths later */
       stop_seconds -= start_seconds;                               /* subtract start time */
       if (stop_seconds < 0)
               stop_seconds += 24 * SECONDS_PER_HOUR;               /* if started before and stopped after midnight */

       if ((diff_hundreds = (int) stop->hundredths - (int) start->hundredths) < 0)
       {
               stop_seconds--;                                      /* borrow a second */
               diff_hundreds += 100;                                /* and add it here */
       }

       diff->hundredths = (char) diff_hundreds;                     /* store it */

       diff->hours = diff->minutes = diff->seconds = 0;

       while (stop_seconds >= SECONDS_PER_HOUR)
       {
               diff->hours++;                                       /* add one hour */
               stop_seconds -= SECONDS_PER_HOUR;                    /* less one hour's worth of seconds */
       }
       while (stop_seconds >= SECONDS_PER_MINUTE)
       {
               diff->minutes++;                                     /* add one minute */
               stop_seconds -= SECONDS_PER_MINUTE;                  /* less one minute's worth of secs */
       }
       diff->seconds = (char) stop_seconds;                         /* remaining seconds * */
}

/*********************************************************************************************************************************
**
** GET_HUNDREDS        -       returns time expressed in hundredths of a second
**
**
**********************************************************************************************************************************/
unsigned long get_hundreds(PDATETIME time)
{

       return (((time->hours * SECONDS_PER_HOUR
                  + time->minutes * SECONDS_PER_MINUTE
                  + time->seconds) * 100) + time->hundredths);
}

