
Architecture of BaseOS :

I am building BaseOS to work on every RISC based Chip ! 
SO , ofcourse , it needs to do all the works which are common in all RISC based Kernels !

Note ---> I did not design the whole architecture 

My way of learning is by asking Questions , like how 0 and 1 can represent some meaningful information  , when a device turned on than from where the code execution starts because we need to put our code there ! 


So my architecure designing is very simple - ask question , see what is common in all RISC based kernels , give required supports in form of queries and responses to extensions and build the thing 

Current Working Module ---> I succesfully had done the initial free regions array populating work ! Than it comes to extensions , our kernel do not know if there is even such thing as a user program exists , it only knows about the extensions , even if user app A asked memory it still will be putted in account of  memory managing extension , so now to implement Process IDs , and give initial memory ranges to extension on boot , we must fetch the extensions from SSD , we have a struct inisde Kernel which stores metadata of extensions like their addresses in SSD memory and their sizes  , so we need to extract data from SSD and I got to know that SSD controllers (MMIO ) differs on every machine  , they have different addresses , different commands and different structure ! SO we need to write SSD data fetch and write drivers for every kind of SSD controller , I thought that let user handle this , user will mount these drivers as extensions but it made a chicken-egg problem for me  , because to load this extension in RAM , I again must fetch data from SSD which is not possible without SSD driver , so I had no option instead of writing drivers for famous SSD controllers which covers 99% of devices ! 
SO I will parse Data from dtb and will turn on the correct SSD driver ! 
Than I see that it will unnecessarily increase my kernel size , because my kernel is built in such a way that it runs on all RISC based chip its size must be minimal so that it runs smooth  even on low memory chips ! 
But as I am making it for all , it has various possibilities hence many if-else blocks and code blocks for different works 
Assume that I wrote 5 SSD drivers but in reality when my OS run on some chip  , it will use one suitable driver only so remaining drivers are just wastage of memory 
Hence I add one more thing in my kernel , it will run on every RISC based chip thats true but it will trim its own size also in the very first boot on any chip  kernel will have the whole hardware information  , for example , it will trim all the unnecessary code ( like those SSD drivers ) from the SSD on the very first boot on any chip , this way the kernel will manage itself in such a way that it fits perfectly on every chip ! 