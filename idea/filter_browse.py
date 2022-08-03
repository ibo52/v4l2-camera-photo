#!/usr/bin/python3
#color codes to terminal to manipulate visual outputs
GREEN = "\033[0;32m"
BLACK = "\033[0;30m"
RED = "\033[0;31m"
BLUE = "\033[0;34m"
PURPLE = "\033[0;35m"
CYAN = "\033[0;36m"
WHITE = "\033[1;37m"

main_color=WHITE
filtered_color=RED

import subprocess as sp
from time import sleep

#---saple image settings---
w=32 #sample image width
h=24 #sample image height
filt_size=8 #filter will be cubic
buff=[x for x in range(w*h)] #sample image buffer
#---saple image settings---

#dump buffer to stdout
def show(buff,w,h):
    for i in range(h):
        for j in range(w):
            print(main_color+f"%4d"%buff[i*w+j],end="")
        print()

#browse buffer and determine filter area	
def showfilt(buff,w,h,fsize):
    for i in range(h):
        for j in range(w):
            for sh in range(fsize):
                for sw in range(fsize):
                    print(filtered_color+ f"%4d"%buff[sh*w+j+sw],end="")
            print()


#function to show how filter browse the image array
def sfb(buff,w,h,fsize):
    
    l=buff.copy()
    #prepare color map
    for i in range(h):
        for j in range(w):
            l[i*w+j]=main_color +f"%4d"%buff[i*w+j]

    #determine filter area and change color of that area
    for i in range(h):
        for j in range(w):
            
            l2=l.copy()
            
            for sh in range(fsize):
                for sw in range(fsize):
                    l2[(i+sh)*w+j+sw]=filtered_color +f"%4d"%buff[(i+sh)*w+j+sw]

            #print processed values to show filter on buffer        
            for m in range(h):
                for n in range(w):
                    print(l2[m*w+n],end="")
                print()
                
            sleep(.08)
            sp.call("clear",shell=True)#clear terminal screen

#show(buff,w,h)
#showfilt(buff,w,h,3)
sfb(buff,w,h,filt_size) #call function to show filtered buffer
