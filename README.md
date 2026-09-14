
BaseOS : A minimalist microkernel 

BaseOS is a bare-metal microkernel , which I am designing for cross-architecture portability across RISC based Chips .
It seperates all the core kernel things , User can easily customize it by mounting extensions on it ! 
For Example : Some Chip has MMU , so the user needs virtual memory and paging  , the user will mount an extension on BaseOS which supports paging and virtual memory !

Architecture and Desgin Philosophy :

Core VS extensions : Yes , it is extension based OS , I am trying my best to make it such so .

Current Status :
 DTB Parsing --> Parsing DTB tree perfectly , extracting RAM information and reserved regions informations also ! 
 Memory Ranges --> Successfully finding free memory ranges and storing in an array which tracks the free vs in-use memory ! 

 Project Structure -->

 C:.
│   .gitignore
│   build.ps1
│   README.md
│   testboard.dts
│   
├───build
│       alignbyte.o
│       boot.elf
│       boot.o
│       dbt.o
│       Image
│       main.o
│       mkmau.o
│       mkmau_utils.o
│       mobilemkMAU.o
│       quicksort.o
│       test_board.dtb
│       trampoline.o
│       uart.o
│       
├───docs
│       architectur.md
│       decisions.md
│       troubleshooting.md
│       
├───include
│   ├───boot
│   ├───driverHeaders
│   │       uart.h
│   │       
│   ├───lib
│   │       alignbyte.h
│   │       dbt.h
│   │       quicksort.h
│   │       
│   └───mkMAU
│           mkMAU.h
│           mkmau_utils.h
│           mobilemkMAU.h
│           
├───linker
│       linker.ld
│       
└───src
    ├───assemblycode
    │       boot.s
    │       
    ├───ccode
    │       main.c
    │       trampoline.c
    │       
    ├───driverCode
    │       uart.c
    │       
    ├───lib
    │       alignbyte.c
    │       dbt.c
    │       quicksort.c
    │       
    └───mkMAU
            mkmau.c
            mkmau_utils.c
            mobilemkMAU.c