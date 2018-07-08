#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include<iostream>
#include<string>
#include "BSFSParams.h"

using namespace std;

struct Process
{
    pid_t pid;
    string pname;
    unsigned int cnt;
    unsigned int flist[MAX_OPEN_CNT_P];
    bool valid;
};

#endif // PROCESS_H_INCLUDED
