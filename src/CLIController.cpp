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
#include "PIController.h"

using namespace std;

CLIController::CLIController(FSController& _fsc, UserController& _uc, VHDController& _vhdc,
                                PIController& _pic, int _uid, const iNode& nowiNode) :
    fsc(_fsc), uc(_uc), vhdc(_vhdc), pic(_pic), uid(_uid)
{
    memcpy((char*)&this->nowiNode, (char*)&nowiNode, sizeof(iNode));
    return;
}

bool CLIController::MakeMenu()
{
    if(!fsc.GetiNodeByID(nowiNode.bid, &nowiNode))
        return false;

    char dir[MAX_CMD_LEN];
    this->fsc.GetAbsDir(nowiNode, dir);
    char uname[MAX_UNAME_LEN];
    if(!this->uc.GetUsernameByUid(uid, uname))
        return false;

    cout << "╭─" << uname << "@" << HOSTNAME << " ";
    cout << dir << endl;
    cout << "╰─> ";
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

void CLIController::GetLastSeg(char* cmd, int len, char* dirname, int &dncnt)
{
    if(cmd[len-1] == '/')
    {
        while(cmd[len-1] == '/') len--;
    }
    for(int i = len-1; i >= 0; i--)
    {
        if(cmd[i] == '/') break;
        dirname[dncnt++] = cmd[i];
    }
    for(int i = 0; i <= (dncnt-1)/2; i++)
        swap(dirname[i], dirname[dncnt-1-i]);
    dirname[dncnt] = '\0';
}

bool CLIController::GetProcessID(char* cmd, int len, pid_t pid)
{
    for(int i = 0; i < len; i++)
    {
        if(cmd[i] < '0' && cmd[i] > '9') return false;
        pid += cmd[i]-'0';
    }
    return true;
}

bool CLIController::ReadCommand(bool &exitFlag)
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
    if(lentmp == 0)
        if(!MakeMenu()) return false;
    if(tmp[lentmp-1] != ' ') tmp[lentmp++] = ' ';

    char cmd[5][MAX_CMD_LEN];
    memset(cmd, 0, sizeof(cmd));
    int len[5] = {0}, cmdp = 1;
    for(int i = 0; i < lentmp; i++)
    {
        if(tmp[i] == ' ')
        {
            cmd[cmdp][len[cmdp]] = '\0';
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
        for(int i = 0; i < subDirnum; i++)
        {
            if(len[2] == 0)
            {
                if(strcmp(DirSet[i].name, ".") == 0) continue;
                if(strcmp(DirSet[i].name, "..") == 0) continue;
            }
            cout.setf(ios::left);
            cout.width(maxWidth + LS_PADDING);
            cout << DirSet[i].name;
            cnt++;
            if(cnt % 5 == 0) cout << endl;
        }
        cout.unsetf(ios::left);
        if(cnt % 5 != 0) cout << endl;
    }
    if(strcmp(cmd[1], "ll") == 0)
    {
        if(len[2] != 0) return false;

        iNode rst;
        char moderst[8];
        char uname[MAX_UNAME_LEN];
        unsigned int totSize = 0;

        for(int i = 0; i < subDirnum; i++)
        {
            if(this->fsc.GetiNodeByID(DirSet[i].inode, &rst))
            {
                for(int j = 0; j < 8; j++) moderst[j] = '-';
                DisplayMode(rst.mode, moderst);
                for(int j = 0; j < 8; j++) cout << moderst[j];
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
        cout << "total " << totSize << endl;
    }
    if(strcmp(cmd[1], "cd") == 0)
    {
        if(len[2] == 0)
        {
            cmd[2][len[2]++] = '.';
            cmd[2][len[2]++] = '\0';
        }
        if(len[3] != 0) return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
            return false;
        if(!(rst.mode & DIRFLAG)) return false;

        memcpy((char*)&nowiNode, (char*)&rst, sizeof(iNode));
    }
    if(strcmp(cmd[1], "openr") == 0)
    {
        if(len[3] == 0) return false;
        pid_t pid = 0;
        if(!GetProcessID(cmd[3], len[3], pid))
            return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
            return false;
        if(rst.mode & DIRFLAG) return false;
        if(!this->pic.CheckXlock(rst.bid)) return false;

        if(!this->pic.FOpen(pid, rst, false))
            return false;
    }
    if(strcmp(cmd[1], "openw") == 0)
    {
        if(len[3] == 0) return false;
        pid_t pid = 0;
        if(!GetProcessID(cmd[3], len[3], pid))
            return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
            return false;
        if(rst.mode & DIRFLAG) return false;
        if(!this->pic.CheckXlock(rst.bid)) return false;

        if(!this->pic.FOpen(pid, rst, true))
            return false;
    }
    if(strcmp(cmd[1], "mkdir") == 0)
    {
        if(len[2] == 0 || len[3] != 0) return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], false, &rst))
            return false;

        char dirname[FILENAME_MAXLEN];
        int dncnt = 0;
        GetLastSeg(cmd[2], len[2], dirname, dncnt);
        if(!fsc.CreateSubDir(rst, dirname, DIR_DEFAULT_FLAG, uid))
            return false;
    }
    if(strcmp(cmd[1], "vim") == 0)
    {}
    if(strcmp(cmd[1], "rm") == 0)
    {
        if(len[2] == 0) return false;
        if((len[3] != 0) && (strcmp(cmd[2], "-r") != 0)) return false;

        iNode rst;
        if(len[3] == 0)
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
                return false;
            if(rst.mode & DIRFLAG) return false;

            if(!this->fsc.DeleteFile(rst))
                return false;
        }
        else
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &rst))
                return false;
            if(!(rst.mode & DIRFLAG)) return false;

            if(!this->fsc.DeleteDir(rst))
                return false;
        }
    }
    if(strcmp(cmd[1], "chmod") == 0)
    {
        if(len[3] == 0) return false;
        if(len[2] != 2) return false;
        for(int i = 0; i < len[2]; i++)
        {
            if(cmd[2][i] < '0' || cmd[2][i] > '9') return false;
        }

        iNode rst;
        int mode = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &rst))
            return false;
        mode = ((cmd[2][0]-'0') << 4) | ((cmd[2][1]-'0') << 1);
        if(rst.mode & DIRFLAG) mode = mode | DIRFLAG;
        if(!this->fsc.ChangeMode(rst, (char)mode))
            return false;
    }
    if(strcmp(cmd[1], "cp") == 0)
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode))
            return false;
        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
                return false;
            GetLastSeg(cmd[3], len[3], newname, newlen);
        }
        else
            strcpy(newname, srciNode.name);

        if(!(desiNode.mode & DIRFLAG)) return false;
        if(!this->fsc.Copy(srciNode, desiNode, newname, this->uid))
            return false;
    }
    if(strcmp(cmd[1], "mv") == 0)
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode))
            return false;
        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
                return false;
            GetLastSeg(cmd[3], len[3], newname, newlen);
        }
        else
            strcpy(newname, srciNode.name);

        if(!(desiNode.mode & DIRFLAG)) return false;
        if(!this->fsc.Move(srciNode, desiNode, newname))
            return false;
    }
    if(strcmp(cmd[1], "touch") == 0)
    {
        if(len[3] != 0) return false;

        iNode rst;
        if(this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
            return false;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], false, &rst))
            return false;

        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        GetLastSeg(cmd[2], len[2], newname, newlen);
        iNode newrst;
        if(!this->fsc.Touch(rst, newname, FILE_DEFAULT_FLAG, uid, &newrst))
            return false;
    }
/*    if(strcmp(cmd[1], "lnh") == 0)
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode))
            return false;
        char linkname[FILENAME_MAXLEN];
        int linklen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
                return false;
            GetLastSeg(cmd[3], len[3], linkname, linklen);
        }
        else strcpy(linkname, srciNode.name);

        if(!this->fsc.LinkH(srciNode, desiNode, linkname))
            return false;
    }
    if(strcmp(cmd[1], "lns") == 0)
    {
        if(len[3] == 0) return false;

        iNode desiNode;
        char linkname[FILENAME_MAXLEN];
        int linklen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
                return false;
        }
        GetLastSeg(cmd[3], len[3], linkname, linklen);

        if(!this->fsc.LinkS(cmd[2], desiNode, linkname))
            return false;
    }
*/
    if(strcmp(cmd[1], "exit") == 0)
    {
        exitFlag = true;
        return true;
    }

    return true;
}
