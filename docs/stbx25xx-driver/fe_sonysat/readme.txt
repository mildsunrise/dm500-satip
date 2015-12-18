General Information of Driver for the Tuner on Redwood6.

Module Name:

       fe_sonysat   -  installable kernel module (insmod fe_sonysat)

Files:

       tuner.c cstuner.c tuner_test.c cstuner.h tuner.h
       
Device Name

       /dev/fe_sonysat

Node

      mknod /dev/fe_sonysat c 250 0

      Major No is 250

Application

      tuner_test  lnb disecq symbolrate freq
                  
                  lnb = 1 power vertical
                        2 power horizontal

                  disecq = 0 22KHZ off
                           1 22KHZ on 

                  symbolrate  = Kbaud

                  freq = MHz


     This test application can be used to program the Sony satellite tuner 
     provided with the STBx25xx evaluation kit.  

---------------------------------------------------------------------------