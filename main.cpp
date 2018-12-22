
#include "dbms.hh"


int main(int argc, char **argv) {
    return Dbms::Main(argc, argv);
}

// TODO: whot with reads/writes in nodes?? -> get read/write file to new class

/*
 * Stuff to implement:
 * * removing records and add it to test files
 *
 * !* experiment
 *
 * Stuff to improve:
 * * AllocateDiskMemory (use some array of free spaces??)
 *
 *
 */