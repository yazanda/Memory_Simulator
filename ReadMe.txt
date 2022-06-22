Memory Simulator
Authored by Yazan Daefi
323139923

==Description==

The program is a simulation of processor approaches to memory (RAM), done using C++, contains load and store chars to an array that descripes the main memory.
It works on demand paging rools, receives a logical address then translate it to physical address to store/load in the main memory.
Creating a frmae in the main memory by search for an empty one or passing the oldest used frame to a swap file in order to empty a new place in the memory.
It can load values from the executable file or the swap file according to the page's type.

Program DATABASE:

1.sim_mem - a class that describes the simulator, contains all the functions and other properties.
2.main_memory – array of chars that describes the memory.
3.page_descriptor – a structure that describes all the pages in the process.
4.availableFrames/usedFrames - queus that contains the numbers of the frames that the program used (works on the principle of FIFO).
5.exec_file1/exec_file2 - files that describes the processes.
6.swap_file - file that describes the swap memory.

functions:
two main functions:
1.store - funcion that receievs process id, logical address and a value to store in the main memory.
  how it workes: it translate the address to a physical address, calculate the page and the offset then checks the page's status.
                 if valid then it exists in the memory and it just stores the new value.
                 if the page has'nt writing permision then the function prints error.
                 if the page is dirty then it reads it from the swap file, passing to the memory and store the new value.
                 if not dirty it checks if it is a data page or bss/heap stack page, if data it reads from the executable file, if bss/heap stack it initializes a new empty page and stores the value. 
2.load - function that receives process id and a logical address to read a value from the main memory.
  how it workes: it translate the address to a physical address, calculate the page and the offset then checks the page's status.
                 if valid then it exists in the memory and it just reads the value.
                 if the page has'nt writing permision then the function reads from the executable file.
                 if the page is dirty then it reads it from the swap file, passing to the memory and read the value.
                 if not dirty it checks if it is a data page or bss/heap stack page, if data it reads from the executable file, if bss/heap stack it prints error.
five help functions:
1.getPageAddress - function that crate frame in the main memory, update the page table and read from files by calling other help functions.
               then it calculates the physical address and returns it.
               receives the process id, the file to read from, the page, the offset and the dirty value.
2.findFrame - function that checks if there are a place in the memory to store/load values.
              create a frame in tha main memory and push it to the queue, receives the process id.
3.swapOut - function that empty the oldest used frame in the main memory and copy it to the swap file, receives the process id.
4.readFromFile - function that reads from a file.
                 receives the process id,the file that we want to read from, the page to find the frame in the file, and the index of the page in the swap file. 
5.isUsedFrame - a boolean function that checks if a frame is in using by any page of any process.
                receives the frame number and the process id.                          
6.errorTel - function that print an error to the screen, calls the destructor and exit the program.                          
                          
==Program Files==
sim_mem.h - the file contains the class and all the definissions of the functions, in addition to the structor.
sim_mem.cpp - contains the functions only.
main.cpp - containes the main(tester).

==How to compile?==
compile: g++ sim_mem.cpp main.cpp -o ex5
run: ./ex5

==Input:==
No input.

==Output:==
According to my tester:
9 pages that been stored to and loaded from the main memory.
The main memory.
The swap file.
The page tables of the processes.
