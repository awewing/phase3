#define MAX_SEMS 200

typedef struct procStruct procStruct;

typedef struct procStruct * procPtr;

struct procStruct {
    int status;
    int pid;
    int ppid;
    int numKids;
}
