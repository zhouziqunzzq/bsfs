#include<cstdio>
#include<cstring>
#include<iostream>
#include<algorithm>
#include<ctime>
#include "CLIController.h"
#include "BSFSParams.h"
#include "FSController.h"
#include "iNode.h"
#include "UserController.h"

using namespace std;

CLIController::CLIController(FSController& _fsc, UserController& _uc, int _uid) :
    fsc(_fsc), uc(_uc), uid(_uid)
{
    return;
}

bool CLIController::MakeMenu()
{
    char dir[MAX_CMD_LEN];
    this->fsc.GetAbsDir(nowiNode, dir);
    char uname[MAX_UNAME_LEN];
    if(!this->uc.GetUsernameByUid(uid, uname))
        return false;

    cout << "--" << uname << "@" << HOSTNAME << " ";
    cout << dir << endl;
    cout << "--> ";
    return true;
}

void CLIController::DisplayMode(const char mode, char* rst)
{
    if(mode & DIRFLAG) rst[0] = 'd';
    if(mode & OWNER_RFLAG) rst[1] = 'r';
    if(mode & OWNER_WFLAG) rst[2] = 'w';
    if(mode & OWNER_XFLAG) rst[3] = 'x';
    if(mode & PUBLIC_RFLAG) rst[4] = 'r';
    if(mode & PUBLIC_WFLAG) rst[5] = 'w';
    if(mode & PUBLIC_XFLAG) rst[6] = 'x';
    if(mode & SYNLINKFLAG) rst[7] = 's';
}

bool CLIController::ReadCommand()
{
    char c, tmp[MAX_CMD_LEN];
    int lentmp = 0;
    while(scanf("%c", &c))
    {
        if(c == '\n') break;
        if(c == ' ' && lentmp > 0 && tmp[lentmp-1] == ' ') continue;
        if(c == ' ' && lentmp == 0) continue;
        tmp[lentmp++] = c;
    }

    char cmd[5][MAX_CMD_LEN];
    int len[5] = {0}, cmdp = 1;
    for(int i = 0; i < lentmp; i++)
    {
        if(tmp[i] == ' ')
        {
            cmd[cmdp][len[cmdp]++] = '\0';
            cmdp++;
            continue;
        }
        cmd[cmdp][len[cmdp]++] = tmp[i];
    }

    int subDirnum = nowiNode.size / sizeof(SFD);
    SFD* DirSet = new SFD[subDirnum];
    if(!this->fsc.GetContentInDir(nowiNode, DirSet))
    {
        delete[] DirSet;
        return false;
    }
    int maxWidth = 0;
    for(int i = 0; i < subDirnum; i++)
        maxWidth = max(maxWidth, (int)strlen(DirSet[i].name));

    if(strcmp(cmd[1], "ls") == 0)
    {
        if((len[2] != 0) && (strcmp(cmd[2], "-a") != 0))
            return false;

        int cnt = 0;
        cout.setf(ios::left);
        cout.width(maxWidth);
        for(int i = 0; i < subDirnum; i++)
        {
            if(len[2] == 0)
            {
                if(strcmp(DirSet[i].name, ".") == 0) continue;
                if(strcmp(DirSet[i].name, "..") == 0) continue;
            }
            cout << DirSet[i].name;
            cnt++;
            if(cnt % 5 == 0) cout << endl;
        }
        cout.unsetf(ios::left);
    }
    if(strcmp(cmd[1], "ll") == 0)
    {
        if(len[2] != 0) return false;

        iNode rst;
        char moderst[8];
        for(int i = 0; i < 8; i++) moderst[i] = '-';
        char uname[MAX_UNAME_LEN];
        unsigned int totSize = 0;

        for(int i = 0; i < subDirnum; i++)
        {
            if(this->fsc.GetiNodeByID(DirSet[i].inode, &rst))
            {
                for(int j = 0; j < 8; j++) cout << moderst[i];
                cout << " ";
                cout << rst.nlink << " ";
                if(!this->uc.GetUsernameByUid(rst.uid, uname))
                    return false;
                else cout << uname << " ";
                cout << rst.size << " ";
                totSize += rst.size;
                tm* t = localtime(&rst.mtime);
                cout << month[t->tm_mon] << " ";
                cout << t->tm_mday << " ";
                cout << t->tm_hour << ":" << t->tm_min << " ";
                cout << DirSet[i].name << endl;
            }
        }
        cout << "total" << totSize << endl;
    }
    if(strcmp(cmd[1], "cd") == 0)
    {}
    if(strcmp(cmd[1], "openr") == 0)
    {}
    if(strcmp(cmd[1], "openw") == 0)
    {}
    if(strcmp(cmd[1], "mkdir") == 0)
    {}
    if(strcmp(cmd[1], "vim") == 0)
    {}
    if(strcmp(cmd[1], "rm") == 0)
    {}
    if(strcmp(cmd[1], "chmod") == 0)
    {}
    if(strcmp(cmd[1], "cp") == 0)
    {}
    if(strcmp(cmd[1], "mv") == 0)
    {}

    return true;
}
