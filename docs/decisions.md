
SO , this file is really important because it will show how I think , so lets start ! 

 1. As soon as I enter in main.c , I must extract dbt info so I extracted it , store it  and than  I treat the dbt memory range as free range casue we had extracted all the important information !
 2. I sort the array which stores the resreved regions memory ranges because data with particular patterns is more easy and efficient to use ! 
 3. The array inside mkMAU which tracks free and inuse memory does not store memory ranges of reserved regions and kernel range , because I felt that it is a better way to hide important things of which most of the extensions have no use ! SO I tried to hide important informations this way by hiding them ! 
  

  // --> 16-09-2026

  1. BaseOS dont know about if user apps exists or not  , it only knows about extensions , if there is some memory managing extension asking for memory on behalf os User App A than it still will be putted in account of that extension ! 
  2. Extensions also runs on kernel privileges , it is the user work to add secure extensions ! 


  