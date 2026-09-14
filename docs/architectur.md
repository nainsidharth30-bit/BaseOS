
Architecture of BaseOS :

I am building BaseOS to work on every RISC based Chip ! 
SO , ofcourse , it needs to do all the works which are common in all RISC based Kernels !

Note ---> I did not design the whole architecture 

My way of learning is by asking Questions , like how 0 and 1 can represent some meaningful information  , when a device turned on than from where the code execution starts because we need to put our code there ! 


So my architecure designing is very simple - ask question , see what is common in all RISC based kernels , give required supports in form of queries and responses to extensions and build the thing 

Current Working Module ---> (mkMAU) micro kernel Memory Allocation Unit  , its whole work is to keep track of free vs in use memory regions  , to give responses to the extensions queries  