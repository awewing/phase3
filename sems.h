#define MAX_SEMS 200

typedef struct procStruct process;
typedef struct procStruct * procPtr;

typedef struct semStruct semaphore;
typedef struct semStruct * semPtr;

struct procStruct {
   procPtr         childProcPtr;
   procPtr         nextSiblingPtr;
//   procPtr         quitChildPtr;
   char            name[MAXNAME];     /* process's name */
   char            startArg[MAXARG];  /* args passed to process */
   short           pid;               /* process id */
   short           ppid;              /* parent process id */
   int             priority;
   int (* start_func) (char *);       /* function where process begins -- launch */
   unsigned int    stackSize;
//   int             status; 
   int             numKids;
   int             mbox;
};

struct semStruct {
    
};
