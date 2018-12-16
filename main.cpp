
#include "dbms.hh"


int main(int argc, char **argv) {
    return Dbms::Main(argc, argv);
}

// TODO: whot with reads/writes in nodes?? -> get read/write file to new class

/*
 * Stuff to implement:
 * * removing records
 * !* iterator
 * !* cache (needed to effective iteration)
 * !* reading data from Test files (CUD operations) done
 *
 *params of db: I/O pages counter, height, nodes number, size, records number, % disk and ram memory utilization (disk util will be good for an experiment)
 * !*
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