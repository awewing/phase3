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
#include <usyscall.h>
/* ------------------------- Prototypes ----------------------------------- */
static void nullsys3(systemArgs *args);
static void spawn(systemArgs *args);
static void wait(systemArgs *args);
static void terminate(systemArgs *args);
static void semCreate(systemArgs *args);
static void semP(systemArgs *args);
static void semV(systemArgs *args);
static void semFree(systemArgs *args);
static void getTimeofDay(systemArgs *args);
static void spuTime(systemArgs *args);
static void getPID(systemArgs *args);

int spawnReal(char *name, int(*func)(char *), char *arg, int stacksize, int priority);
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

static void syscallHandler(int dec, void *args);
void check_kernel_mode(char* name);
/* ------------------------- Globals ------------------------------------ */
void (*sys_vec[MAXSYSCALLS])(systemArgs *args);

process ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];

int numProcs = 3;
int current = 3;
/* ------------------------- Functions ------------------------------------ */
int start2(char *arg) {
    int pid;
    int status;
    
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
    sys_vec[SYS_SPAWN] = Spawn;
    sys_vec[SYS_WAIT] = Wait;
    sys_vec[SYS_TERMINATE] = Terminate;
    sys_vec[SYS_SEMCREATE] = SemCreate;
    sys_vec[SYS_SEMP] = SemP;
    sys_vec[SYS_SEMV] = SemV;
    sys_vec[SYS_SEMFREE] = SemFree;
    sys_vec[SYS_GETTIMEOFDAY] = GetTimeofDay;
    sys_vec[SYS_CPUTIME] = CPUTime;
    sys_vec[SYS_GETPID] = GetPID;

    // initialize ProcTable
    for (int i = 0; i < MAXPROC; i++) {
        ProcTable[i].name[0] = '\0';
        ProcTable[i].startArg[MAXARG] = '\0';
        ProcTable[i].ppid = -1;
        ProcTable[i].priority = -1;
        ProcTable[i].start_func = NULL;
        ProcTable[i].stackSize = -1;
        ProcTable[i].status = -1;
        ProcTable[i].numKids = -1;
    }

    // initialize SemTable
    for (int i = 0; i < MAXSEMS; i++) {
    }

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
    pid = spawnReal("start3", start3, NULL,  4 * USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

} /* start2 */

static void nullsys3(systemArgs *args) {
    USLOSS_Console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    USLOSS_Halt(1); // TODO change this from halt to terminate
}

static void spawn(systemArgs *args) {
    // values to send to spawn real
    char *name;
    int (*func)(char *);
    char *arg;
    int stacksize;
    int priority;

    // check values are real
    

    // all values good, assign them
    name = args.arg5;
    func = args.arg1;
    arg  = args.arg2;
    stacksize = args.arg3;
    priority = args.arg4;

    // call spawn real
    spawnReal(name, func, arg, stacksize, priority);    
}

static void wait(systemArgs *args) {
    waitReal();
}

static void terminate(systemArgs *args) {
    terminateReal();
}

static void semCreate(systemArgs *args) {
    semCreateReal();
}

static void semP(systemArgs *args) {
    semPReal();
}

static void semV(systemArgs *args) {
    semVReal();
}

static void semFree(systemArgs *args) {
    semFreeReal();
}

static void getTimeofDay(systemArgs *args) {
    getTimeofDayReal();
}

static void cpuTime(systemArgs *args) {
    cpuTimeReal();
}

static void getPID(systemArgs *args) {
    getPIDReal();
}

int spawnReal(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    ProcTable[]

    int kidpid = fork1(name, spawnLaunch, NULL, stacksize, priority);
    // put kidpid in process table?
}

int spawnLaunch(char *args) {
    int result;

    if (!isZapped()) {
        result = ProcTable[getpid() % MAXPROC].start_func(ProcTable[getpid() % MAXPROC].startArg);
    
        terminate(result);
    }
}

int waitReal(int *status) {

}

int terminateReal(int status) {

}

int semCreateReal(systemArgs *args) {

}

int semPReal(systemArgs *args) {

}

int semVReal(systemArgs *args) {

}

int semFreeReal(systemArgs *args) {

}

int getTimeofDayReal(systemArgs *args) {

}

int cpuTimeReal(systemArgs *args) {

}

int getPIDReal(systemArgs *args) {

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
 * Checks if we are in Kernel mode
 */
void check_kernel_mode(char *name) {
    if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("%s(): Called while in user mode by process %d. Halting...\n", name, getpid());
        USLOSS_Halt(1);
    }
}
