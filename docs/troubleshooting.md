
I started my project nearly 3 days ago , so see 

1 . First problem which I tackle is byte alignment for RISC based architectures , I was parsing dtb tree , my logic was perfect but somehow data is wrong so I got to know that bytes alignment is very important in RISC , and that dtb tree is also stored in byte aligned format so I did manage byte alignemnet 

2 . After dbt , I enterd in mkMAU where I need a data structure which will track the free and in use memory ranges ! The problem I tackeld here is size of that data structure !  I made a sruct mkMAU_node which stores start and end address of some memory range and a flag also which tells if it is free or in-use ! I implememted a limit of 4kb memory , if any extension ever want memory than the extension must ask for atleast 4kb of memory  , so I need to calculate maximum numbe of mkmau_nodes possible so that we can give a fixed size to  the memory_tracker_array_mkmau ! 
But ram info , free amount of memory and all other infos are calculated on runtime , so it absolutely was not possible to make a static array , than I thought of many structures  , like using a linked list , using a tree  , but tree consumes unnecessary memory and linkedlist is dynamic so I was afraid that what if it is increased enormously and overwrites some resereved region  ! 
SO I thought of building a dynamic array , a dynamic array can also overwrite something else , but I did find a way to this Problem -->
 On runtime I calculated maximum  number of mkmau_nodes hence maximum size the memory_tracker_array_needs , I traverse through the whole RAM and FInd a range which perfectly fits my array , this way , with a little memory OverHead , I did it ! 

 Upto --->>   ///  15-09-2026 ////////


Some imporvements in DTB parsing

// ---> 16-09-2026
  

    

