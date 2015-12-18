/*----------------------------------------------------------------------------+
|
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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
// An silly test of helper-queue
// Sept/13/2001, YYD

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#include "os/helper-queue.h"


int main(void)
{
   QUEUE_T  q1, q2;

   USHORT buf[256];

   srand(1201321);

   printf("Create queue 1 return : %d\n",  os_create_queue(&q1, buf, 256, sizeof(USHORT)));

   printf("Create queue 2 return : %d\n",  os_create_queue(&q2, NULL, 1000, sizeof(ULONG)));

   {
     USHORT i, k;
     USHORT j;
     for (i=0; i<256; i++)
     {
        j = i + ((~i)<<8);
        if(os_enqueue(&q1, &j) < 0)
        {
            printf("Queue 1 full at %d elements\n", k=i);
            break;
        }
     }
     for(k,i=0; k>0; k--, i++)
     {
        if(os_dequeue(&q1, &j) < 0)
        {
            printf("unexpected empty queue\n");
            break;
        }
        if (j != (USHORT)(i + ((~i)<<8)))
        {
            printf("Dequeue error %d\n", i);
        }
     }
   }
   {
       ULONG i,j,k;
       
       for(i=0; i<700; i++) os_enqueue(&q2, &i);
       for(i=0; i<200; i++) os_dequeue(&q2, &j);
       for(i=0; i<498; i++) os_enqueue(&q2, &i);
       if(os_enqueue(&q2, &i) < 0)
       {
          printf("should not full\n");
       }
       if(os_enqueue(&q2, &i) >= 0)
       {
           printf("Should full\n");
       }

       k = 0;
       for(i=0; i<23400; i++)
       {
          if(os_dequeue(&q2, &i) >= 0) k++;
       }
       printf("Trying to dequeue 999 elements get %d elements\n", k);

       for(i=0; i<2340; i++)
       {

           for(j=rand()%100; j; j--) os_enqueue(&q2, &i);
           for(j=rand()%100; j; j--) os_dequeue(&q2, &k);
       }
       printf("Queue status = %d\n", os_get_queue_status(&q2));

       for(i=0; i<1024; i++) os_enqueue(&q2, &i);

       os_dequeue(&q2, &i);

       k = 0xbeef;
       os_enqueue(&q2, &k);
       while(os_dequeue(&q2, &k) >= 0);
       printf("K should be 0xbeef, and is 0x%4.4x\n", k);
   }

   os_delete_queue(&q1);
   os_delete_queue(&q2);

   
}
