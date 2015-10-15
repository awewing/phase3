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
/* ------------------------- Prototypes ----------------------------------- */
extern int start3(char *);

static void nullsys3(systemArgs *args);
static void spawn(systemArgs *args);
static void wait(systemArgs *args);
static void terminate(systemArgs *args);
static void semCreate(systemArgs *args);
static void semP(systemArgs *args);
static void semV(systemArgs *args);
static void semFree(systemArgs *args);
static void getTimeofDay(systemArgs *args);
static void cpuTime(systemArgs *args);
static void getPID(systemArgs *args);

int spawnReal(char *name, int(*func)(char *), char *arg, unsigned int stackSize, int priority);
int spawnLaunch(char *args);
int waitReal(int *status);
void terminateReal(int status);
int semCreateReal(systemArgs *args);
int semPReal(systemArgs *args);
int semVReal(systemArgs *args);
int semFreeReal(systemArgs *args);
void getTimeofDayReal(systemArgs *args);
void cpuTimeReal(systemArgs *args);
void getPIDReal(systemArgs *args);

void addChild(int parentID, int childID);
static void syscallHandler(int dec, void *args);
void check_kernel_mode(char* name);
/* ------------------------- Globals ------------------------------------ */
int debugflag3 = 1;
void (*sys_vec[MAXSYSCALLS])(systemArgs *args);

process ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];

int ptMutex;
int stMutex;
int childDone;

int ptCom;

int numProcs = 3;
int current = 3;
/* ------------------------- Functions ------------------------------------ */
int start2(char *arg) {
    // Check kernel mode here.
    check_kernel_mode("start2");
    
    // Data structure initialization as needed...
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler; 

    // initialize sys_vec
    // initialize everything to nullsys3
    for (int i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }

    // fill specified ones with their specific functions
    sys_vec[SYS_SPAWN] = spawn;
    sys_vec[SYS_WAIT] = wait;
    sys_vec[SYS_TERMINATE] = terminate;
    sys_vec[SYS_SEMCREATE] = semCreate;
    sys_vec[SYS_SEMP] = semP;
    sys_vec[SYS_SEMV] = semV;
    sys_vec[SYS_SEMFREE] = semFree;
    sys_vec[SYS_GETTIMEOFDAY] = getTimeofDay;
    sys_vec[SYS_CPUTIME] = cpuTime;
    sys_vec[SYS_GETPID] = getPID;

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
        ProcTable[i].numKids = -1;
        ProcTable[i].mbox = MboxCreate(MAXPROC, MAXLINE);
    }

    // initialize SemTable
    for (int i = 0; i < MAXSEMS; i++) {
    }

    // create mutex mailbox
    ptMutex = MboxCreate(1, 0);
    stMutex = MboxCreate(1, 0);
    //childQuit = MboxCreate(50, 0

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
    USLOSS_PsrSet(0);

    int pid;
    pid = spawnReal("start3", start3, NULL,  4 * USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    int status;
    if (pid > 0) {
        pid = waitReal(&status);
    }

    return status;
} /* start2 */

static void nullsys3(systemArgs *args) {
    USLOSS_Console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    terminateReal(1);
}

static void spawn(systemArgs *args) {
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
    stacksize = *((int *)args->arg3);
    priority = *((int *)args->arg4);

    // call spawn real
    int pid = spawnReal(name, func, arg, stacksize, priority);    

    args->arg1 = &pid;
    args->arg4 = 0;
}

static void wait(systemArgs *args) {
    int *code;
    int result = waitReal(code);

    args->arg1 = &result;
    args->arg2 = code;

    int success = 0;
    if (ProcTable[getpid()].numKids == 0) {
        success = -1;
    }
    
    args->arg4 = &success;
}

static void terminate(systemArgs *args) {
    int code = *((int *)args->arg1);
    terminateReal(code);
}

static void semCreate(systemArgs *args) {
//    semCreateReal();
}

static void semP(systemArgs *args) {
//    semPReal();
}

static void semV(systemArgs *args) {
//    semVReal();
}

static void semFree(systemArgs *args) {
//    semFreeReal();
}

static void getTimeofDay(systemArgs *args) {
//    getTimeofDayReal();
}

static void cpuTime(systemArgs *args) {
//    cpuTimeReal();
}

static void getPID(systemArgs *args) {
//    getPIDReal();
}

int spawnReal(char *name, int(*func)(char *), char *arg, unsigned int stackSize, int priority) {
    MboxSend(ptMutex, NULL, 0);
    int kidpid = fork1(name, spawnLaunch, arg, stackSize, priority);

    int slot = kidpid % MAXPROC;
    strcpy(ProcTable[slot].name, name);
    if (arg == NULL) {
        ProcTable[slot].startArg[0] = '\0';
    }
    else {
        memcpy(ProcTable[slot].startArg, arg, MAXARG);
    }
    ProcTable[slot].priority = priority;
    ProcTable[slot].start_func = func;
    ProcTable[slot].stackSize = stackSize;
    ProcTable[slot].ppid = getpid();
    addChild(getpid(), kidpid);

    MboxSend(ProcTable[slot].mbox, NULL, 0);
    MboxReceive(ptMutex, NULL, 0);
    return kidpid;
}

int spawnLaunch(char *args) {
    process me = ProcTable[getpid() % MAXPROC];

    me.pid = getpid();

    MboxReceive(me.mbox, NULL, 0);

    int result = -1;
    if (!isZapped()) {
        result = me.start_func(me.startArg);
    
        terminateReal(result);
    }

    return result;
}

int waitReal(int *status) {
    return join(status);
}

void terminateReal(int status) {
    if (ProcTable[getpid() % MAXPROC].numKids == 0) {
        quit(status);
        return;
    }

    // go through all children
    for (int i = 0; i < ProcTable[getpid()].numKids; i++) {
        // zap child
        zap(ProcTable[getpid()].childProcPtr->pid);
        
        // remove child
        ProcTable[getpid()].childProcPtr = ProcTable[getpid()].childProcPtr->nextSiblingPtr;
    }

    // remove this process from its parents list of processes
    procPtr parent = &ProcTable[ProcTable[getpid()].ppid];

    for (procPtr child = parent->childProcPtr; child->nextSiblingPtr != NULL; 
                                                child = child->nextSiblingPtr) {
        // find yourself in your parents child list
        if (child->nextSiblingPtr->pid == getpid()) {
            // derefrence yourself in the list
            child->nextSiblingPtr = child->nextSiblingPtr->nextSiblingPtr;

            // dec the parents kids
            parent->numKids--;

            break;
        }
    }

    // wait for all children to to finish terminating before leaving
    //can we even mbox receive? how will children send?

    // quit
    quit(status);
}

int semCreateReal(systemArgs *args) {
    return 0;
}

int semPReal(systemArgs *args) {
    return 0;
}

int semVReal(systemArgs *args) {
    return 0;
}

int semFreeReal(systemArgs *args) {
    return 0;
}

void getTimeofDayReal(systemArgs *args) {
    return;
}

void cpuTimeReal(systemArgs *args) {
    return;
}

void getPIDReal(systemArgs *args) {
    return;
}

void addChild(int parentID, int childID) {
    ProcTable[parentID].numKids++;

    if (ProcTable[parentID].childProcPtr == NULL) {
        ProcTable[parentID].childProcPtr = &ProcTable[childID];
    }
    else {
        procPtr child;
        for (child = ProcTable[parentID].childProcPtr; child->nextSiblingPtr != NULL; child = child->nextSiblingPtr) {}

        child->nextSiblingPtr = &ProcTable[childID];
    }
}

static void syscallHandler(int dev, void *args) {
    // get args
    systemArgs *sysPtr = (systemArgs *) args;

    // check if valid dev
    if (dev != USLOSS_SYSCALL_INT) {
        USLOSS_Console("syscallHandler(): Bad call\n");
        USLOSS_Halt(1);
    }

    // check if valid range of args
    if (sysPtr->number < 0 || sysPtr->number >= MAXSYSCALLS) {
        USLOSS_Console("syscallHandler(): sys number %d is wrong.  Halting...\n", sysPtr->number);
        USLOSS_Halt(1);
    }

    USLOSS_PsrSet( USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
    sys_vec[sysPtr->number](sysPtr);
}
/*
 * setUserMode
 *
 *  Will basically change the current mode
 *     user --> kernel
 *     kernel --> user
 */
void setUserMode() {
    //TODO: test

    unsigned int psr = USLOSS_PsrGet();
    unsigned int newPsr = psr ^ 00000001;

    if (debugflag3) {
        USLOSS_Console("curr psr = %d\n", psr);
        USLOSS_Console("new psr = %d\n", newPsr);
    }
    USLOSS_PsrSet(newPsr);
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