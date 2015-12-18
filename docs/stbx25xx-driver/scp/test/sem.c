/*---------------------------------------------------------------------------+
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
#include <sys/sem.h>


int CreateSemaphore(int );
int SemPost( int);
int SemWait( int);
void DeleteSemaphore( int );

int CreateSemaphore(int count )
{
  int semid;

  #if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
  union semun parm                /* union is defined by including <sys/sem.h>*/
  #else
  /* according to X/OPEN we have to define it ourselves */
  union semun {
        int val;                  /* value for SETVAL */
        struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
        unsigned short *array;    /* array for GETALL, SETALL */
                                  /* Linux specific part: */
        struct seminfo *__buf;    /* buffer for IPC_INFO */
  } parm;
  #endif

  semid = semget( IPC_PRIVATE, 1, 0664 | IPC_CREAT | IPC_EXCL );
  if( semid < 0 )
    return(-1);
  parm.val = count;
  if( semctl( semid, 0, SETVAL, parm) < 0)
  {
    DeleteSemaphore( semid );
    return(-1);
  }
  return(semid);
}

void DeleteSemaphore( int semid )
{
  semctl( semid, IPC_RMID, 0 );
  return;
}

int SemPost( int semid )
{
  struct sembuf sem;

  sem.sem_num = 0;
  sem.sem_op = 1;
  sem.sem_flg = 0;

  if(semop( semid, &sem, 1 ) == -1 )
    return(-1);
  else
    return(0);
}

int SemWait( int semid )
{
  struct sembuf sem;

  sem.sem_num = 0;
  sem.sem_op = -1;
  sem.sem_flg = 0;

  if( semop( semid, &sem, 1 ) == -1 )
    return(-1);
  else
    return(0);
}


