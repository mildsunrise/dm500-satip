//pallas/drv/os/test/pm-alloc-test.c
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
//
//Comment: 
//  Physical memory block allocation routines tester
//Revision Log:   
//  Sept/03/2001            Created by YYD

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#include "os/pm-alloc.h"

#include "../pm-alloc-local.h"  // I'm tester, so I need to know some internels

static int reverse_int(int b, int n)
{
  int i,k;
  k=b&1;
  for(i=1; i<n; i++) { k<<=1; b>>=1; k|= b&1; }
  return k; 
}


int main()
{
    void *ebuf = malloc(9*1024*1024 + 512*1024);  // 1k boundary aligned, plus the extra needed by handle pool

    void *paddress = (void *)((((UINT)ebuf + 1024 - 1)/(1024)) * (1024));
    
    printf("Malloced emulation buffer at 0x%8.8x\n", paddress);
    printf("Init pm_alloc return %d\n", __os_alloc_physical_init((UINT)paddress, 8*1024*1024 + 512*1024)); fflush(stdout);

    printf("Get_P_Base = 0x%8.8x,  Get_Total =0x%8.8x\n", 
        os_get_physical_base(), os_get_physical_total_size());
    fflush(stdout);
    
    // try to allocate some blocks
    {
        int i, j, k;
        MEM_HANDLE_T h[256];

        srand(3432432);
        printf("Allocate test\n"); fflush(stdout);
           
        for(i=0; i<256; i++)
        {
            j = rand()%(16384);
            printf("[%03d] Try to allocate %d bytes : ", i, j); fflush(stdout);
            h[i] = os_alloc_physical(j);
            printf("Returned 0x%8.8x\n", h[i]);
            if(NULL != h[i])
            {
                printf("   l_addr=0x%8.8x, p_addr=0x%8.8x, bsize=0x%8.8x\n",
                    os_get_logical_address(h[i]), os_get_physical_address(h[i]), h[i]->uSize);
            }
            fflush(stdout);
            __os_alloc_physical_heap_walk();
        }
        printf("\n\nFree test\n"); fflush(stdout);
        for(i=0; i<256; i++)
        {
            j = reverse_int(i, 8);
            printf("[%03d] Free %d = 0x%8.8x .. ",i, j, h[j]);
            os_free_physical(h[j]);
            printf("done.\n");
            fflush(stdout);
            __os_alloc_physical_heap_walk();
        }
        printf("\n\nLarge alloc/free test.\n"); fflush(stdout);
        for(i=1; i<9; i++)
        {
            j = i*1024*1024;
            printf("[%03d] Try to allocate/free %d MBytes\n",i, i); fflush(stdout);
            h[i] = os_alloc_physical(j);
            printf("  Allocate returned 0x%8.8x\n", h[i]);
            if(NULL != h[i])
            {
                printf("   l_addr=0x%8.8x, p_addr=0x%8.8x, bsize=0x%8.8x\n",
                        os_get_logical_address(h[i]), os_get_physical_address(h[i]), h[i]->uSize);
               }
            fflush(stdout);
            printf("  Try to free it .. ");
            os_free_physical(h[i]);
            printf("done.\n"); fflush(stdout);
            __os_alloc_physical_heap_walk();
        }

        printf("\n\nJustified alloc/free test.\n"); fflush(stdout);

        for(i=1024*1024; i>0; i>>=1)
        {
            printf(" Try to allocate/free 1 MBytes, justified by 0x%08x\n",i); fflush(stdout);
            h[0] = os_alloc_physical_justify(1024*1024, i);
            if(NULL != h[0])
            {
                printf("   l_addr=0x%8.8x, p_addr=0x%8.8x, bsize=0x%8.8x\n",
                        os_get_logical_address(h[0]), os_get_physical_address(h[0]), h[0]->uSize);
            }
            fflush(stdout);
            printf("  Try to free it .. ");
            os_free_physical(h[0]);
            printf("done.\n"); fflush(stdout);
            __os_alloc_physical_heap_walk();
        }

        for(i=1024*1024; i>0; i>>=1)
        {
            printf(" Try to allocate/free 1 MBytes, justified by 0x%08x\n", i+123); fflush(stdout);
            h[0] = os_alloc_physical_justify(1024*1024, i+123);
            if(NULL != h[0])
            {
                printf("   l_addr=0x%8.8x, p_addr=0x%8.8x, bsize=0x%8.8x\n",
                        os_get_logical_address(h[0]), os_get_physical_address(h[0]), h[0]->uSize);
            }
            fflush(stdout);
            printf("  Try to free it .. ");
            os_free_physical(h[0]);
            printf("done.\n"); fflush(stdout);
            __os_alloc_physical_heap_walk();
        }



        printf("\n\nOptimized allocation test.\n"); fflush(stdout);
        printf("  1. Make size decreasing holes.\n"); fflush(stdout);
        memset(h, 0, sizeof(h)); // make sure every one is cleaned
        
        #define MAXBLOCK 2048   // try to be on 2's exp boundary !!
        for(i=0,k=MAXBLOCK; k>0; i+=2, k>>=1)
        {
            j = k*1024;
            printf("     [%03d] Try to allocate %d kB x 2\n",i/2, k); fflush(stdout);
            h[i] = os_alloc_physical(j);
            h[i+1] = os_alloc_physical(j);
            printf("        Allocate returned 0x%8.8x, 0x%8.8x\n", h[i], h[i+1]);
            fflush(stdout);
        }
        __os_alloc_physical_heap_walk();
            for(i-=2; i>=0; i-=2)
            {
                os_free_physical(h[i]);
                h[i]=NULL;
            }
        __os_alloc_physical_heap_walk();

        printf("  2. Try to alloc size increased, if allocation is optimized, no one should fail.\n"); fflush(stdout);
        for(i=0,k=1; k<MAXBLOCK; i+=2, k= (k<<1)+1)
        {
            j = k*1024;
            printf("     [%03d] Try to allocate %d kB\n",i/2, k); fflush(stdout);
            h[i] = os_alloc_physical(j);
            if(NULL == h[i])
            {
                printf("        *******  Failed to allocate such a big size!\n");
            }
            else
                printf("        Allocate returned 0x%8.8x, p_addr=0x%8.8x, size=0x%8.8x\n", h[i], os_get_physical_address(h[i]),h[i]->uSize);
            fflush(stdout);
            __os_alloc_physical_heap_walk();
        }
        // free up them before next test to leave space for overrun
        for(i=0,k=1; k<MAXBLOCK; i+=2, k= (k<<1)+1)
        {
            os_free_physical(h[i]);
            h[i] = NULL;
        }
        

        printf("\n\nBuffer overrun detection test.\n"); fflush(stdout);
        printf("  1. Overrun all allocated buffers from previous steps.\n"); fflush(stdout);
        for(i=0; i<256; i++)
        {
            BYTE *pOver;
            if(NULL == h[i]) continue;
            pOver = os_get_logical_address(h[i]) + h[i]->uSize*__PM_ALLOC_UNIT;  // internel constant
            printf("   Blowing handle 0x%8.8x, start l_addr= 0x%8.8x\n", h[i], pOver);
            for(k=0; k<256; k++) pOver[k] = rand()&0xff;
            fflush(stdout);
        }
        printf("  2. Overrun will be detected within heap walk.\n"); fflush(stdout);
        __os_alloc_physical_heap_walk();

        // ok, clean up all my stuff
        printf("Cleanning up\n"); fflush(stdout);
        for(i=0; i<256; i++)
        {
             os_free_physical(h[i]);   // make sure everyone is freed
        }
        __os_alloc_physical_heap_walk();
            
    }


    printf("Leave some allocated mem before deinit.\n");
    os_alloc_physical(1234567);
    __os_alloc_physical_heap_walk();
    printf("Deinit it \n"); fflush(stdout);
    __os_alloc_physical_deinit();
    printf("Ok, test done\n"); fflush(stdout);
    free(ebuf);
    return 0;
}
