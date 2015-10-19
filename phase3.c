/*------------------------------------------------------------------------
   phase3.c

   University of Arizona
   Computer Science 452

   @Authors
    -Andre Takagi
    -Alex Ewing
  ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <sems.h>
#include <usyscall.h>
#include <libuser.h>
/* ------------------------- Prototypes ----------------------------------- */
extern int start3(char *);

static void nullsys3(systemArgs *args);
static void spawn(systemArgs *args);
static void waitt(systemArgs *args);
static void terminate(systemArgs *args);
static void semCreate(systemArgs *args);
static void semP(systemArgs *args);
static void semV(systemArgs *args);
static void semFree(systemArgs *args);
static void getTimeOfDay(systemArgs *args);
static void cpuTime(systemArgs *args);
static void getPID(systemArgs *args);

int spawnReal(char *name, int(*func)(char *), char *arg, unsigned int stackSize, int priority);
int spawnLaunch(char *args);
int waitReal(long *status);
void terminateReal(int status);
int semCreateReal(int value);
int semPReal(int semID);
int semVReal(int semID);
int semFreeReal(int semID);
int getTimeOfDayReal();
int cpuTimeReal();
int getPIDReal();

void addChild(int parentID, int childID);
void removeChild(int parentID, int childID);

void setUserMode();
void check_kernel_mode(char* name);
int getNextSem();
/* ------------------------- Globals ------------------------------------ */
int debugflag3 = 0;
int debugflag3v = 0;

process ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];

int ptMutex;
int stMutex;

int numProcs = 3;
int current = 3;

int numSems = 0;
int nextSem = 0;
/* ------------------------- Functions ------------------------------------ */
int start2(char *arg) {
    // Check kernel mode here.
    check_kernel_mode("start2");
    
    // Data structure initialization as needed...
    // initialize sys_vec
    // initialize everything to nullsys3
    for (int i = 0; i < MAXSYSCALLS; i++) {
        systemCallVec[i] = nullsys3;
    }

    // fill specified ones with their specific functions
    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_WAIT] = waitt;
    systemCallVec[SYS_TERMINATE] = terminate;
    systemCallVec[SYS_SEMCREATE] = semCreate;
    systemCallVec[SYS_SEMP] = semP;
    systemCallVec[SYS_SEMV] = semV;
    systemCallVec[SYS_SEMFREE] = semFree;
    systemCallVec[SYS_GETTIMEOFDAY] = getTimeOfDay;
    systemCallVec[SYS_CPUTIME] = cpuTime;
    systemCallVec[SYS_GETPID] = getPID;

    // initialize ProcTable
    for (int i = 0; i < MAXPROC; i++) {
        ProcTable[i].childProcPtr = NULL;
        ProcTable[i].nextSiblingPtr = NULL;
        ProcTable[i].name[0] = '\0';
        ProcTable[i].startArg[0] = '\0';
        ProcTable[i].pid = -1;
        ProcTable[i].ppid = -1;
        ProcTable[i].priority = -1;
        ProcTable[i].start_func = NULL;
        ProcTable[i].stackSize = -1;
        ProcTable[i].numKids = 0;
        ProcTable[i].spawnBox = MboxCreate(0, MAXLINE);
        ProcTable[i].quitBox = MboxCreate(MAXPROC, MAXLINE);
    }

    // initialize SemTable
    for (int i = 0; i < MAXSEMS; i++) {
        SemTable[i].mutexBox = -1;
        SemTable[i].blockedBox = -1;
        SemTable[i].value = 0;
        SemTable[i].blocked = 0;
    }

    // create mutex mailbox
    ptMutex = MboxCreate(1, 0);
    stMutex = MboxCreate(1, 0);

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    int pid;
    pid = spawnReal("start3", start3, NULL,  4 * USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    long status;
    if (pid > 0) {
        pid = waitReal(&status);
    }

    return status;
} /* start2 */

static void nullsys3(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: nullsys3\n", getpid());
    }

    USLOSS_Console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    terminateReal(1);
}

static void spawn(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: spawn\n", getpid());
    }

    // values to send to spawn real
    char *name;
    int (*func)(char *);
    char *arg;
    int stacksize;
    int priority;

    // check values are real
        // if not set args4 = -1 and return

    // all values good, assign them
    name = args->arg5;
    func = args->arg1;
    arg  = args->arg2;
    stacksize = (long) args->arg3;
    priority = (long) args->arg4;

    // call spawn real
    long pid = spawnReal(name, func, arg, stacksize, priority);    

    // send the return info back
    args->arg1 = (void *) pid;
    args->arg4 = (void *) 0L;

    // switch back to usermode
    setUserMode();
}

static void waitt(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: waitt\n", getpid());
    }

    // create space to save an int
    long code;

    // pass a pointer of that int to waitReal
    long result = waitReal(&code);

    // a variabale that checks if the wait was successfull
    long success = 0;

    // if the waited on process had no kids it wasn't successful, set that
    if (result == -2) {
        success = -1;
    }

    // set the result of waitReal, code, and success to the system args
    args->arg1 = (void *) result;
    args->arg2 = (void *) code;
    args->arg4 = (void *) success;

    // switch back to user mode
    setUserMode();
}

static void terminate(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: terminate\n", getpid());
    }

    // get the quit code from the system args
    int code = (long) args->arg1;

    // quit with given code
    terminateReal(code);

    // switch back to user mode
    setUserMode();
}

static void semCreate(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: semCreate\n", getpid());
    }

    // real call
    long address = semCreateReal((long) args->arg1);

    if (address == -1) {
        args->arg4 = (void *) -1L;
        args->arg1 = NULL;
    } else {
        args->arg4 = 0;
        args->arg1 = (void *) address;
    }
}

static void semP(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: semP\n", getpid());
    }
    
    // input
    int semID = (long) args->arg1;

    // real call
    long result = semPReal(semID);

    // output
    args->arg4 = (void *) result;
}

static void semV(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: semV\n", getpid());
    }

    // input
    int semID = (long) args->arg1;

    // real call
    long result = semVReal(semID);

    // output
    args->arg4 = (void *) result;
}

static void semFree(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: semFree\n", getpid());
    }

    int semID = (long) args->arg1;

    if (semID == -1) {
        args->arg4 = (void *) -1L;
    } else {
        long result = semFreeReal(semID);
        args->arg4 = (void *) result;
    }
}

static void getTimeOfDay(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: getTimeOfDay\n", getpid());
    }

    long result = getTimeOfDayReal();
    args->arg1 = (void *) result;
}

static void cpuTime(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: cpuTime\n", getpid());
    }

    long result = cpuTimeReal();
    args->arg1 = (void *) result;
}

static void getPID(systemArgs *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: getPID\n", getpid());
    }

    long result = getPIDReal();
    args->arg1 = (void *) result;
}

int spawnReal(char *name, int(*func)(char *), char *arg, unsigned int stackSize, int priority) {
    if (debugflag3) {
        USLOSS_Console("process %d: spawnReal\n", getpid());
    }

    // block anyone from doing stuff with the process table
    MboxSend(ptMutex, NULL, 0);
    int kidpid = fork1(name, spawnLaunch, arg, stackSize, priority);

    // add parameters to child's slot in the processtable
    int slot = kidpid % MAXPROC;
    ProcTable[slot].pid = kidpid;
    strcpy(ProcTable[slot].name, name);
    if (arg != NULL) {
        strcpy(ProcTable[slot].startArg, arg);
    }
    ProcTable[slot].priority = priority;
    ProcTable[slot].start_func = func;
    ProcTable[slot].stackSize = stackSize;
    ProcTable[slot].ppid = getpid();

    // add child to the cirrent process
    addChild(getpid(), kidpid);

if(debugflag3v) USLOSS_Console("    %d, %d\n", ProcTable[slot].pid, ProcTable[slot].ppid);


if(debugflag3v) USLOSS_Console("    process %d: sending message to %d\n", getpid(), kidpid);

    // announce that you set the child's values
    MboxSend(ProcTable[slot].spawnBox, NULL, 0);

if(debugflag3v) USLOSS_Console("    process %d: sent message\n", getpid());

    // release the mutex
    MboxReceive(ptMutex, NULL, 0);
   
    return kidpid;
}

int spawnLaunch(char *args) {
    if (debugflag3) {
        USLOSS_Console("process %d: spawnLaunch\n", getpid());
    }

if(debugflag3v) USLOSS_Console("    process %d: receiving message\n", getpid());

    // block and allow the parent to set our values
    MboxReceive(ProcTable[getpid() % MAXPROC].spawnBox, NULL, 0);

    // get ourselves in the process table
    process me = ProcTable[getpid() % MAXPROC];

if(debugflag3v) USLOSS_Console("    process %d: received message from %d\n", getpid(), me.ppid);

    // set up a return value
    int result = -1;

    // if we weren't zapped run normally and quit
    if (!isZapped()) {
        // switch to usermode
        setUserMode();

        // get the arguments we need to do stuff will
        int (*func)(char *) = me.start_func;
        char arg[MAXARG];
        strcpy(arg, me.startArg);

        // run the function with its args
        result = (func)(arg);

        // quit
        Terminate(result);
    }
    // otherwise, just quit
    else {
        terminateReal(0);
    }

    return result;
}

int waitReal(long *status) {
    if (debugflag3) {
        USLOSS_Console("process %d: waitReal\n", getpid());
    }

/*
    // check for no children
    if (ProcTable[getpid() % MAXPROC].numKids == 0) {
        return -1;
    }
*/

    int result = join(status);
    return result;
}

void terminateReal(int status) {
    if (debugflag3) {
        USLOSS_Console("process %d: terminateReal\n", getpid());
    }

    // get the current process
    process me = ProcTable[getpid() % MAXPROC];

    // if you have children, get rid of them
    if (me.numKids != 0) {

        // go through all children
        for (int i = 0; i < me.numKids; i++) {
            // zap child
            zap(me.childProcPtr->pid);
        
            // remove child
            me.childProcPtr = me.childProcPtr->nextSiblingPtr;
        }

        // remove this process from its parents list of processes
        procPtr parent = &ProcTable[me.ppid];
        for (procPtr child = parent->childProcPtr; child->nextSiblingPtr != NULL; 
             child = child->nextSiblingPtr) {
            // find yourself in your parents child list
            if (child->nextSiblingPtr->pid == getpid()) {
                // derefrence yourself in the list
                child->nextSiblingPtr = child->nextSiblingPtr->nextSiblingPtr;

                // dec the parents kids
                parent->numKids--;

                // leave the loop
                break;
            }
        }
    
        // wait for all children to to finish terminating before leaving
        for (int i = 0; i < me.numKids; i++) {
            MboxReceive(me.quitBox, NULL, 0);
        }
    }

    // send to parent that you quit and remove yourself from their list of children
    int ppid = me.ppid;
    MboxSend(ProcTable[ppid].quitBox, NULL, 0);
    removeChild(me.ppid, me.pid);

    // clear out this processes process table
    int slot = me.pid % MAXPROC;
    ProcTable[slot].childProcPtr = NULL;
    ProcTable[slot].nextSiblingPtr = NULL;
    ProcTable[slot].name[0] = '\0';
    ProcTable[slot].startArg[0] = '\0';
    ProcTable[slot].pid = -1;
    ProcTable[slot].ppid = -1;
    ProcTable[slot].priority = -1;
    ProcTable[slot].start_func = NULL;
    ProcTable[slot].stackSize = -1;
    ProcTable[slot].numKids = 0;
    MboxRelease(ProcTable[slot].spawnBox);
    MboxRelease(ProcTable[slot].spawnBox);
    ProcTable[slot].spawnBox = MboxCreate(0, MAXLINE);
    ProcTable[slot].quitBox = MboxCreate(MAXPROC, MAXLINE);

    // quit
    quit(status);
}

int semCreateReal(int value) {
    if (debugflag3) {
        USLOSS_Console("process %d: semCreateReal\n", getpid());
    }

    // Check maxsems
    if (numSems >= MAXSEMS) {
        return -1;
    }

    // attempt to create a mutex mbox for the semaphore
    int mutexBox = MboxCreate(1, 0);

    if (mutexBox == -1) {
        //no mutexbox created.
        return -1;
    } 

    // attempt to create mbox for P blocks
    int blockedBox = MboxCreate(0, 0);

    if (blockedBox == -1) {
        // no blockedbox created
        return -1;
    }

    // New entry in sem table
    numSems++;
    int sem = getNextSem();
    SemTable[sem].mutexBox = mutexBox;
    SemTable[sem].blockedBox = blockedBox;
    SemTable[sem].value = value;
    SemTable[sem].blocked = 0;
    return sem;
}

int semPReal(int semID) {
    if (debugflag3) {
        USLOSS_Console("process %d: semPReal\n", getpid());
    }

    if (SemTable[semID].mutexBox == -1) {
        return -1;
    }

    // get handles for mboxes in the semaphore struct
    int mutexBox = SemTable[semID].mutexBox;
    int blockedBox = SemTable[semID].blockedBox;
    
    // enter critical section
    MboxSend(mutexBox, NULL, 0);
    if (debugflag3) {
        USLOSS_Console("process %d: in critical section\n", getpid());
    }

    // while there is not a resource to grab (value = 0)
    while ( SemTable[semID].value <= 0 ) {
        // must block ourselves
        SemTable[semID].blocked++;

        // exit critical section to allow semV to happen
        MboxReceive(mutexBox, NULL, 0);

        // block on zero slot mailbox, will meet up with semV here
        if (debugflag3) {
            USLOSS_Console("process %d: blocking on P\n", getpid());
        }
        MboxSend(blockedBox, NULL, 0);

        if (debugflag3) {
            USLOSS_Console("process %d: Unblocked on P\n", getpid());
        }
        // return to critical section
        MboxSend(mutexBox, NULL, 0);
        if (debugflag3) {
            USLOSS_Console("process %d: in critical section\n", getpid());
        }
    }

    // take resource (decrement v)
    SemTable[semID].value--;
    // exit critical section
    MboxReceive(mutexBox, NULL, 0);

    return 0;
}

int semVReal(int semID) {
    if (debugflag3) {
        USLOSS_Console("process %d: semVReal\n", getpid());
    }

    if (SemTable[semID].mutexBox == -1) {
        return -1;
    }

    // get handles for mboxes in the semaphore struct
    int mutexBox = SemTable[semID].mutexBox;
    int blockedBox = SemTable[semID].blockedBox;
    
    // enter critical section
    MboxSend(mutexBox, NULL, 0);
    if (debugflag3) {
        USLOSS_Console("process %d: in critical section\n", getpid());
    }

    // increment value
    SemTable[semID].value++;

    // Unblock any proc blocked on P
    if (SemTable[semID].blocked > 0) {
        // may have to wait for semP here
        if (debugflag3) {
            USLOSS_Console("process %d: freeing process blocked on P\n", getpid());
        }
        MboxReceive(blockedBox, NULL, 0);
        SemTable[semID].blocked--;
    }

    MboxReceive(mutexBox, NULL, 0);
    return 0;
}

int semFreeReal(int semID) {
    if (debugflag3) {
        USLOSS_Console("process %d: semFreeReal\n", getpid());
    }

    if (SemTable[semID].mutexBox == -1) {
        return -1;
    }

    MboxRelease(SemTable[semID].mutexBox);
    MboxRelease(SemTable[semID].blockedBox);

    SemTable[semID].mutexBox = -1;
    SemTable[semID].blockedBox = -1;
    SemTable[semID].value = -1;
    SemTable[semID].blocked = 0;

    numSems--;
    return 0;
}

int getTimeOfDayReal() {
    if (debugflag3) {
        USLOSS_Console("process %d: getTimeOfDayReal\n", getpid());
    }

    return readtime();
}

int cpuTimeReal() {
    if (debugflag3) {
        USLOSS_Console("process %d: cpuTimeReal\n", getpid());
    }

    return -1;
}

int getPIDReal() {
    if (debugflag3) {
        USLOSS_Console("process %d: getPIDReal\n", getpid());
    }

    return getpid();
}

void addChild(int parentID, int childID) {
    if (debugflag3) {
        USLOSS_Console("process %d: addChild\n", getpid());

        if(debugflag3v) {
            int kids = ProcTable[parentID % MAXPROC].numKids;
            USLOSS_Console("    parent: %d, child: %d\n", parentID, childID);
            USLOSS_Console("    before add: parent has %d kids\n", kids);
        }
    }

    // readjust the input to align with the table
    parentID %= MAXPROC;
    childID %= MAXPROC;

    // inc the number of kids
    ProcTable[parentID].numKids++;

    // if the parent has no kids, set the child as the parents child
    if (ProcTable[parentID].childProcPtr == NULL) {
        ProcTable[parentID].childProcPtr = &ProcTable[childID];
    }
    // if the parent does have kids, find its last kid and append the child to that last kids's next
    else {
        procPtr child;
        for (child = ProcTable[parentID].childProcPtr; child->nextSiblingPtr != NULL; 
             child = child->nextSiblingPtr) {}

        child->nextSiblingPtr = &ProcTable[childID];
    }
}

void removeChild(int parentID, int childID) {
    if (debugflag3) {
        USLOSS_Console("process %d: removeChild\n", getpid());
        
        if(debugflag3v) {
            int kids = ProcTable[parentID % MAXPROC].numKids;
            USLOSS_Console("    parent: %d, child: %d\n", parentID, childID);
            USLOSS_Console("    before remove: parent has %d kids\n", kids);
        }
    }

    // readjust the input to align with the table
    parentID %= MAXPROC;
    childID %= MAXPROC;

    // dec the number of kid
    ProcTable[parentID].numKids--;

    // find the child and derefrence them
    // if the child is the parent's first child
    if (ProcTable[parentID].childProcPtr->pid == childID) {
        // swap the parent's first child with its next sibling
        ProcTable[parentID].childProcPtr = ProcTable[parentID].childProcPtr->nextSiblingPtr;
    }
    // otherwise
    else {
        procPtr child;
        for (child = ProcTable[parentID].childProcPtr; child->nextSiblingPtr != NULL; 
             child = child->nextSiblingPtr) {
            if (child->nextSiblingPtr->pid == childID) {
                child->nextSiblingPtr = child->nextSiblingPtr->nextSiblingPtr;
            }
        }
    }
}

/*
 * setUserMode:
 *  kernel --> user
 */
void setUserMode() {
    USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE);
}

/*
 * Checks if we are in Kernel mode
 */
void check_kernel_mode(char *name) {
    if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("%s(): Called while in user mode by process %d. Halting...\n", name, getpid());
        USLOSS_Halt(1);
    }
}

/*
 * Get next semaphore on the semaphore table.
 */
int getNextSem() {
    while (SemTable[nextSem].mutexBox != -1) {
        nextSem++;
        if (nextSem >= MAXSEMS) {
            nextSem = 0;
        }
    }

    return nextSem;
}
