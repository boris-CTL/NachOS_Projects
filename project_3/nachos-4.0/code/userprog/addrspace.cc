// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

bool AddrSpace::pages_being_used[NumPhysPages] = {0};

AddrSpace::AddrSpace()
{
    NUMBER_ = (*((*kernel).machine)).number_ + 1 ;
    (*((*kernel).machine)).number_ = (*((*kernel).machine)).number_ + 1;
    
    
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------
//bor
bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;

    unsigned int size;
    unsigned int need_to_find;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;


   pageTable = new TranslationEntry[numPages];

    size = numPages * PageSize;

    
    int s;
    if (noffH.code.size > 0) { 
        for(int k = 0; k <= (numPages-1) ; k++){
            s = 0;
            while((s <= (NumPhysPages-1)) && ((*((*kernel).machine)).frames_that_are_used[s] == 1)) {
                s = s + 1;
            }

            pageTable[k].dirty = 0;
            pageTable[k].NUMBER_ = NUMBER_;
            pageTable[k].use = 0;
            pageTable[k].readOnly = 0;

            if( s >= NumPhysPages){
                pageTable[k].valid = 0;

                need_to_find = 0;
                while((*((*kernel).machine)).virtual_pages_that_are_used[need_to_find] == 1){
                    need_to_find = need_to_find + 1;
                }

                pageTable[k].virtualPage = need_to_find;                
                (*((*kernel).machine)).virtual_pages_that_are_used[need_to_find] = 1;
                char *array_related_to_pages = new char[PageSize];                
                int _position = (PageSize*k) + noffH.code.inFileAddr;
                (*executable).ReadAt(array_related_to_pages, PageSize, _position);
                (*((*kernel).backing_store)).WriteSector(need_to_find, array_related_to_pages);  
            }
            else{
                pageTable[k].counter_ = pageTable[k].counter_ + 1;            
                pageTable[k].valid = 1;

                pageTable[k].physicalPage = s;
                (*((*kernel).machine)).frames_that_are_used[s] = 1;
                (*((*kernel).machine)).frame_number[s] = NUMBER_;
                (*((*kernel).machine)).table_for_translation[s] = &pageTable[k];
                
                int _position_ = (PageSize*k) + noffH.code.inFileAddr;
                char * _address = &((*((*kernel).machine)).mainMemory[PageSize*s]);
                (*executable).ReadAt(_address,PageSize, _position_);                         
            }
        }
    }


	if (noffH.initData.size > 0) {
        executable->ReadAt(
		&(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void AddrSpace::Execute(char *fileName) 
{
    
    check_for_loading = 0;
    if (!Load(fileName)) {
	    cout << "inside !Load(FileName)" << endl;
	    return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register
    
    check_for_loading = 1;

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++) {
        machine->WriteRegister(i, 0);
    }
	

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    
    if (check_for_loading == 1) {
        pageTable = kernel->machine->pageTable;
        numPages = kernel->machine->pageTableSize;
    }
    
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
