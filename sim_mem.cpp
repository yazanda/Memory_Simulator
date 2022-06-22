#include "sim_mem.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

char main_memory[MEMORY_SIZE];
/**********Constructor**********/
sim_mem::sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name[],
                 int text_size, int data_size, int bss_size, int heap_stack_size,
                 int num_of_pages, int page_size, int num_of_process) {
    //initializing the class values.
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->num_of_pages = num_of_pages;
    this->page_size = page_size;
    this->num_of_proc = num_of_process;
    int text_pages = this->text_size/ this->page_size;//number of the text pages.
    int memoryLogicSize = this->page_size*(this->num_of_pages-text_pages);//swap file size.
    switch (num_of_process) {//opening the files.
        case 2:
            if((this->program_fd[0] = open(exe_file_name1, O_RDONLY, 0)) == -1 ||
               (this->program_fd[1] = open(exe_file_name2, O_RDONLY, 0)) == -1)
                errorTel("opening exe file failed");
            break;
        case 1:
            if((this->program_fd[0] = open(exe_file_name1, O_RDONLY, 0)) == -1)
                errorTel("opening exe file failed");
            break;
        default:
            errorTel("num of processes is not valid");
            break;
    }
    //opening swap file.
    if((this->swapfile_fd = open(swap_file_name, O_CREAT | O_RDWR , S_IRWXU | S_IRWXG | S_IRWXO)) == -1)
        errorTel("opening swap file failed");
    //fill the main memory with zeros.
    for(char & i : main_memory)
        i = '0';
    this->swapPages = new int[memoryLogicSize];
    for(int i = 0; i < memoryLogicSize; i++)
        this->swapPages[i] = -1;
    //initializing the page table.
    this->page_table = (page_descriptor**) calloc(this->num_of_proc,sizeof(page_descriptor*));
    if(this->page_table == nullptr)
        errorTel("Allocating failed");
    for(int i = 0; i < this->num_of_proc; i++) {
        this->page_table[i] = (page_descriptor *) calloc(this->num_of_pages, sizeof(page_descriptor));
        if(this->page_table[i] == nullptr) errorTel("Allocating failed");
        //initializing the page tables with starting values.
        for(int k = 0; k < this->num_of_pages; k++){
            this->page_table[i][k].V = 0;
            this->page_table[i][k].D = 0;
            k < text_pages ? this->page_table[i][k].P = 0 : this->page_table[i][k].P = 1;
            this->page_table[i][k].frame = -1;
            this->page_table[i][k].swap_index = -1;
        }
    }
    //array of zeros to fill in the swap file.
    for(int i = 0; i < memoryLogicSize*(this->num_of_proc); i++){//fill the swap file with zeros.
        if(write(this->swapfile_fd,"0",1) == -1)
            errorTel("writing to swap file failed");
    }
}
/**********Destructor**********/
sim_mem::~sim_mem() {
    if(close(this->program_fd[0]) == -1 ||
      (this->num_of_proc == 2 && close(this->program_fd[1]) == -1) ||
       close(this->swapfile_fd) == -1) //closing all the files.
        perror("closing file failed");
    for(int i = 0;i < this->num_of_proc; i++)//free all the memory allocated.
        free(this->page_table[i]);
    free(this->page_table);
    delete[] this->swapPages;
}
/**********Load Function**********/
char sim_mem::load(int process_id, int address) {
    int page = address / this->page_size; //physical address.
    int offset = address % this->page_size;
    int data_page = (this->text_size+ this->data_size)/ this->page_size;
    int bss_page = data_page + (this->bss_size/ this->page_size);
    int hs_page = bss_page + (this->heap_stack_size/ this->page_size);
    if(page >= this->num_of_pages || address < 0)
        fprintf(stderr,"address is not valid");
    int physicalAddress, curProc = process_id-1;
    if(this->page_table[curProc][page].V == 1) {//if valid, so it exists in the main memory.
        physicalAddress = (this->page_table[curProc][page].frame * page_size) + offset;
        return main_memory[physicalAddress];
    }
    else if(this->page_table[curProc][page].P == 0) {//if text pages.
        physicalAddress = this->getPageAddress(process_id, this->program_fd[curProc], page, this->page_table[curProc][page].D) + offset;
        return main_memory[physicalAddress];
    }
    else if(this->page_table[curProc][page].D == 0) {//not dirty.
        if (page >= this->text_size/this->page_size && page < data_page) { //check if the page is data page.
            physicalAddress = this->getPageAddress(process_id, this->program_fd[curProc], page, this->page_table[curProc][page].D) + offset;
            return main_memory[physicalAddress];
        } else if(page >= data_page && page < bss_page){//if bss page.
            physicalAddress = this->getPageAddress(process_id, -1, page, 1) + offset;
            return main_memory[physicalAddress];
        } else if (page >= bss_size && page < hs_page) { //if heap stack page.
            fprintf(stderr,"Can't load unstored bss/heap stack page");
            return '\0';
        }
    }else {//if dirty.
        physicalAddress = this->getPageAddress(process_id, this->swapfile_fd, page,this->page_table[curProc][page].D) + offset;
        return main_memory[physicalAddress];
    }
    return '\0';
}
/**********Store Function**********/
void sim_mem::store(int process_id, int address, char value) {
    int page = address / this->page_size;
    int offset = address % this->page_size;
    int data_page = (this->text_size+ this->data_size)/ this->page_size;
    if(page >= this->num_of_pages || address < 0)
        fprintf(stderr,"address is not valid");
    int physicalAddress, curProc = process_id-1;
    if(this->page_table[curProc][page].V == 1 && this->page_table[curProc][page].P == 1) {//if the page is valid, so it exists in the main memory.
        physicalAddress = (this->page_table[curProc][page].frame * this->page_size) + offset;
        main_memory[physicalAddress] = value;
    }
    else if(this->page_table[curProc][page].P == 0) {//if the page is text type.
        fprintf(stderr,"Store in text page");
    }
    else if(this->page_table[curProc][page].D == 0) {//not dirty.
        if (page >= this->text_size/ this->page_size && page < data_page) { //check if the page is data page.
            physicalAddress = this->getPageAddress(process_id, this->program_fd[curProc], page, 1) + offset;
            main_memory[physicalAddress] = value;
        } else {//if bss/heap stack page.
            physicalAddress = this->getPageAddress(process_id,-1, page, 1) + offset;
            main_memory[physicalAddress] = value;
        }
    } else {//if dirty.
            physicalAddress = this->getPageAddress(process_id, this->swapfile_fd, page, 1) + offset;
            main_memory[physicalAddress] = value;
        }
}
/**********help Functions**********/
/*
 * function that checks if there are a place in the memory to store/load values.
 * create a frame in tha main memory and push it to the queue.
 */
void sim_mem::findFrame(int process_id) {
    int check;
    int founded = 0;
    for(int i = 0; i < MEMORY_SIZE; i += this->page_size){
        check = 0;
        for(int k = i; k < this->page_size + i; k++){
            if(main_memory[k] == '0')
                check++;
        }
        //check if the frame is empty and not in used.
        if(check == this->page_size && !isUsedFrame(process_id, i/ this->page_size)){
            founded = 1;
            availableFrames.push(i/ this->page_size);
            break;
        }
    }
    if(founded == 0){//if the memory is full, then empty the first used frame to the swap file.
        availableFrames.push(usedFrames.front());
        this->swapOut(process_id);
        usedFrames.pop();
    }
}
/*
 * function that checks if a frame is in using by any page of any process.
 * receives the frame number and the process id.
 */
bool sim_mem::isUsedFrame(int process_id, int frame) {
    int curProc = process_id-1, otherProc;
    if(curProc == 1) otherProc = 0;
    else otherProc = 1;
    for(int i = 0; i < this->num_of_pages; i++){
        if(this->page_table[curProc][i].frame == frame || (this->num_of_proc == 2 && this->page_table[otherProc][i].frame == frame))
            return true;
    }
    return false;
}
/*
 * function that empty the oldest used frame in the main memory and copy it to the swap file.
 */
void sim_mem::swapOut(int process_id) {
    char fromMemory[this->page_size+1];
    int ufr = usedFrames.front();
    int pageOfFrame = -1, swapPage;
    int curProc = process_id-1, otherProc;
    if(curProc == 1) otherProc = 0;
    else otherProc = 1;
    //find an empty page in the swap file.
    for(swapPage = 0; this->swapPages[swapPage] != -1; swapPage++);
    //find the first used frame in the page table.
    for(int i = 0; i < this->num_of_pages; i++) {
        if (this->page_table[curProc][i].frame == ufr) {
            this->page_table[curProc][i].V = 0;
            this->page_table[curProc][i].frame = -1;
            this->page_table[curProc][i].swap_index = swapPage;
            pageOfFrame = i;
            break;
        }
        if (this->num_of_proc == 2 && this->page_table[otherProc][i].frame == ufr) {//if the frame exists in the second process.
            this->page_table[otherProc][i].V = 0;
            this->page_table[otherProc][i].frame = -1;
            this->page_table[otherProc][i].swap_index = swapPage;
            pageOfFrame = i;
            break;
        }
    }
    this->swapPages[swapPage] = pageOfFrame;
    //if the page is dirty, so we move it to the swap file.
    if(this->page_table[curProc][pageOfFrame].D == 1){
        int s = 0;
        for(int k = ufr * this->page_size; s < this->page_size; k++, s++){
            fromMemory[s] = main_memory[k];
            main_memory[k] = '0';
        }
        fromMemory[s] = '\0';
        //moving the pointer in the swap file to the wanted frame.
        lseek(swapfile_fd, swapPage* this->page_size,SEEK_SET);
        if((write(swapfile_fd,fromMemory, this->page_size)) == -1)
            perror("writing to swap failed");
    }
    else{//if not dirty, so the page is not in the swap file, and we just empty a frame in the main memory.
        for(int k = ufr * this->page_size; k < (ufr+1)*this->page_size; k++)
            main_memory[k] = '0';
    }
}
/*
 * function that reads from a file.
 * receives the process id,the file that we want to read from, the page to find the frame in the file, and the index of the page in the swap file.
 */
void sim_mem::readFromFile(int process_id, int page, int file, int swapInd) {
    int directory = file != this->swapfile_fd ? page : swapInd;
    char buff[this->page_size];
    for(int i = 0; i < this->page_size; i++)
        buff[i] = '0';
    lseek(file,directory* this->page_size,SEEK_SET);//move the pointer to the right page.
    if(read(file, buff, this->page_size) == -1)
        errorTel("reading from file failed");
    this->page_table[process_id-1][page].swap_index = -1;
    int memoryPlace = this->page_table[process_id-1][page].frame* this->page_size;
    for(int i = 0; i < this->page_size; i++) {//copy the buffer to the main memory(in the right frame).
        main_memory[memoryPlace] = buff[i];
        memoryPlace++;
    }
    for(int i = 0; i < this->page_size; i++) buff[i] = '0';
    if(file == this->swapfile_fd){
        lseek(file,directory* this->page_size,SEEK_SET);
        if(write(file, buff, this->page_size) == -1)
            perror("writing to swap file failed");
        this->swapPages[swapInd] = -1;
    }
}
/*
 * function that crate frame in the main memory, update the page table and read from files by calling other help functions.
 * then it calculates the physical address and returns it.
 * receives the process id, the file to read from, the page, the offset and the dirty value.
 */
int sim_mem::getPageAddress(int process_id, int file, int page, int dirtyVal) {
    this->findFrame(process_id); //create frame.
    int frame = this->availableFrames.front();
    this->usedFrames.push(frame);
    this->availableFrames.pop();
    this->page_table[process_id-1][page].frame = frame;//update the page table.
    this->page_table[process_id-1][page].V = 1;
    this->page_table[process_id-1][page].D = dirtyVal;
    if(file != -1){
        if(file != this->swapfile_fd)
            this->readFromFile(process_id,page, file, -1);//reading from file.
        else this->readFromFile(process_id, page, file, this->page_table[process_id-1][page].swap_index);
    }
    int physicalAddress = (this->page_table[process_id-1][page].frame) * this->page_size;
    return physicalAddress;
}
/*
 * function that print an error to the screen, calls the destructor and exit the program.
 */
void sim_mem::errorTel(const char error[]) {
    perror(error);
    this->~sim_mem();
    exit(EXIT_FAILURE);
}
/**********print Functions**********/
void sim_mem::print_memory() {
    int i;
    printf("\nPhysical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
void sim_mem::print_swap() {
    char* str = (char *)malloc(this->page_size *sizeof(char));
    int i;
    printf("\nSwap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); //go to the start of the file.
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}
void sim_mem::print_page_table() {
    int i;
    for (int j = 0; j < num_of_proc; j++) {
        printf("\npage table of process: %d \n", j);
        printf("Valid\t Dirty\t Permission\t Frame\t Swap index\n");
        for(i = 0; i < num_of_pages; i++) {
            printf("[%d]\t     [%d]\t [%d]\t     [%d]\t [%d]\n",
                   page_table[j][i].V, page_table[j][i].D,
                   page_table[j][i].P, page_table[j][i].frame,
                   page_table[j][i].swap_index);
        }
    }
}