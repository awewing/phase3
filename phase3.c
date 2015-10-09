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
static void Spawn(systemArgs *args);
static void Wait(systemArgs *args);
static void Terminate(systemArgs *args);
static void SemCreate(systemArgs *args);
static void SemP(systemArgs *args);
static void SemV(systemArgs *args);
static void SemFree(systemArgs *args);
static void GetTimeofDay(systemArgs *args);
static void CPUTime(systemArgs *args);
static void GetPID(systemArgs *args);

int SpawnReal(char *name, int(*func)(char *), char *arg, int stacksize, int priority);
int SpawnLaunch(char *args);
int WaitReal(int *status);
void TerminateReal(int status);
int SemCreateReal(systemArgs *args);
int SemPReal(systemArgs *args);
int SemVReal(systemArgs *args);
int SemFreeReal(systemArgs *args);
void GetTimeofDayReal(systemArgs *args);
void CPUTimeReal(systemArgs *args);
void GetPIDReal(systemArgs *args);

static void syscallHandler(int dec, void *args);
void check_kernel_mode(char* name);
/* ------------------------- Globals ------------------------------------ */
void (*sys_vec[MAXSYSCALLS])(systemArgs *args);

//process table
//semtable

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
    pid = SpawnReal("start3", start3, NULL,  4 * USLOSS_MIN_STACK, 3);

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

static void Spawn(systemArgs *args) {
    SpawnReal(args.arg5, args.arg1, args.arg2, args.arg3, args.arg4);    
}

static void Wait(systemArgs *args) {
    WaitReal();
}

static void Terminate(systemArgs *args) {
    TerminateReal();
}

static void SemCreate(systemArgs *args) {
    SemCreateReal();
}

static void SemP(systemArgs *args) {
    SemPReal();
}

static void SemV(systemArgs *args) {
    SemVReal();
}

static void SemFree(systemArgs *args) {
    SemFreeReal();
}

static void GetTimeofDay(systemArgs *args) {
    GetTimeofDayReal();
}

static void CPUTime(systemArgs *args) {
    CPUTimeReal();
}

static void GetPID(systemArgs *args) {
    GetPIDReal();
}

int SpawnReal(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    int kidpid = fork1(name, spawnlaunch, arg, stacksize, priority);
    // put kidpid in process table?
}

int SpawnLaunch(char *args) {
    // create
}

int WaitReal(int *status) {

}

int TerminateReal(int status) {

}

int SemCreateReal(systemArgs *args) {

}

int SemPReal(systemArgs *args) {

}

int SemVReal(systemArgs *args) {

}

int SemFreeReal(systemArgs *args) {

}

int GetTimeofDayReal(systemArgs *args) {

}

int CPUTimeReal(systemArgs *args) {

}

int GetPIDReal(systemArgs *args) {

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
