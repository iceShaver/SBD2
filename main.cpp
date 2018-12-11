
#include "dbms.hh"


int main(int argc, char **argv) {
    return Dbms().main(argc, argv);
}


/*
 * Stuff to implement:
 * * removing records
 * !* iterator
 * !* cache (needed to effective iteration)
 * !* reading data from test files (CUD operations)
 *
 *params of db: I/O pages counter, height, nodes number, size, records number, % disk and ram memory utilization (disk util will be good for an experiment)
 * !*
 * !* printing db file
 * !* experiment
 * *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 * Stuff to improve:
 * * AllocateDiskMemory (use some array of free spaces??)
 *
 *
 */