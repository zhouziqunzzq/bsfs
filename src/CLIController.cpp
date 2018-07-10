#include<cstdio>
#include<cstring>
#include<iostream>
#include<algorithm>
#include<ctime>
#include<ncurses.h>
#include<unistd.h>
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

bool CLIController::GetProcessID(char* cmd, int len, pid_t& pid)
{
    for(int i = 0; i < len; i++)
    {
        if(cmd[i] < '0' || cmd[i] > '9') return false;
        pid = pid * 10 + cmd[i]-'0';
    }
    return true;
}

void CLIController::VimInit(int row)
{
    int rowmin = line[row].ymin;
	int rowmax = line[row].ymax;
	if(row < xmin || row > xmax)
	{
		rowmin = 0;
		rowmax = VIM_MAX_Y;
	}
	if(rowmax < rowmin) rowmax = VIM_MAX_Y;
	for(int i = rowmin; i <= rowmax; i++)
		mvaddch(row, i, ' ');
}

void CLIController::VimRetkey(int ch, int& ret)
{
    if(ch == KEY_ESC)
		ret = 1;
	else if(ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT)
		ret = 2;
	else if(ch == KEY_BAC)
		ret = 3;
	else ret = 4;
}

void CLIController::VimDir()
{
    VimInit(VIM_LOG_X);
	if(ch == KEY_UP)
	{
		if(x > xmin)
		{
			x--;
			y = min(y, line[x].ymax+1);
		}
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key up(x: %d, y: %d, msg: %d)", x, y, msg[x][y-line[x].ymin]);
	}
	if(ch == KEY_DOWN)
	{
		if(x < xmax)
		{
			x++;
			y = min(y, line[x].ymax+1);
		}
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key up(x: %d, y: %d, msg: %d)", x, y, msg[x][y-line[x].ymin]);
	}
	if(ch == KEY_LEFT)
	{
		if(y > line[x].ymin) y--;
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key up(x: %d, y: %d, msg: %d)", x, y, msg[x][y-line[x].ymin]);
	}
	if(ch == KEY_RIGHT)
	{
		if(y <= line[x].ymax) y++;
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key up(x: %d, y: %d, msg: %d)", x, y, msg[x][y-line[x].ymin]);
	}
}

void CLIController::VimBac()
{
    VimInit(VIM_LOG_X);
	mvprintw(VIM_LOG_X, VIM_LOG_STY, "key backspace(x: %d, y: %d)", x, y);
	if(x == xmin && y == line[x].ymin) return;

	if(y > line[x].ymin)
	{
		msg[x][y-line[x].ymin-1] = 0;
		y--;

		for(int i = y-line[x].ymin; i <= line[x].ymax-line[x].ymin-1; i++)
			msg[x][i] = msg[x][i+1];
		msg[x][line[x].ymax-line[x].ymin] = 0;

		VimInit(x);
		line[x].ymax--;
		for(int i = 0; i <= line[x].ymax - line[x].ymin; i++)
			mvaddch(x, i + line[x].ymin, msg[x][i]);
	}
	else if(y == line[x].ymin)
	{
		x--;
		y = line[x].ymax + 1;

		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key backspace(x: %d, y: %d, msg: %d)", x, y, msg[x][y-line[x].ymin]);

		if(line[x+1].ymax >= line[x+1].ymin)
		{
			VimInit(VIM_LOG_X);
			mvprintw(VIM_LOG_X, VIM_LOG_STY, "%d %d %d %d", line[x+1].ymin, line[x+1].ymax, msg[x][line[x].ymax+1], msg[x+1][0]);

			for(int i = 0; i <= line[x+1].ymax - line[x+1].ymin; i++)
			{
				msg[x][line[x].ymax - line[x].ymin + i + 1] = msg[x + 1][i];
//				mvaddch(x, line[x].ymax+i+1, msg[x+1][i]);
			}
			line[x].ymax = line[x].ymax + line[x + 1].ymax - line[ x + 1].ymin + 1;
			msg[x][line[x].ymax + 1] = '\0';
		}

		for(int i = x + 1; i <= xmax - 1; i ++)
		{
			VimInit(i);
			for(int j = 0; j <= line[i + 1].ymax - line[i + 1].ymin; j++)
				msg[i][j] = msg[i + 1][j];

			line[i].ymin = line[i + 1].ymin;
			line[i].ymax = line[i + 1].ymax;
			msg[i][line[i].ymax + 1]='\0';
		}

		VimInit(xmax);
		for(int i = 0; i <= line[xmax].ymax - line[xmax].ymin; i++)
			msg[xmax][i] = 0;
		msg[xmax][0] = '\0';
		line[xmax].ymin = 1;
		line[xmax].ymax = 0;
		xmax--;

		for(int i = x; i <= xmax; i++)
		{
			VimInit(i);
			for(int j = 0; j <= line[i].ymax - line[i].ymin; j++)
				mvaddch(i, j + line[i].ymin, msg[i][j]);
		}
	}
}

void CLIController::VimLet()
{
    VimInit(VIM_LOG_X);
	mvprintw(VIM_LOG_X, VIM_LOG_STY, "key %d, x: %d, y: %d, xmax: %d", ch, x, y, xmax);

	if(ch == KEY_ENT)
	{
		if(xmax - xmin + 1 == VIM_MAX_X) return;
		if(y != line[x].ymax + 1) return;

		VimInit(x + 1);
		xmax++;
		for(int i = xmax; i >= x + 2; i--)
		{
			VimInit(i);
			for(int j = 0; j <= line[i - 1].ymax - line[i - 1].ymin; j++)
			{
				msg[i][j] = msg[i - 1][j];
				mvaddch(i, j + line[i].ymin, msg[i][j]);
				line[i].ymin = line[i - 1].ymin;
				line[i].ymax = line[i - 1].ymax;
			}
		}

		for(int i = 0; i <= line[x + 1].ymax - line[x + 1].ymin; i++)
			msg[x + 1][i] = 0;
		line[x + 1].ymin = 1;
		line[x + 1].ymax = 0;
		x++;
		y = line[x].ymax + 1;
		VimInit(VIM_LOG_X);
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "x: %d, y: %d, ymax: %d, xmax: %d", x, y, line[x].ymin, line[x].ymax);
		return;
	}

	if(y > VIM_MAX_Y) return;

	if(msg[x][y - line[x].ymin] != 0)
	{
		for(int i = line[x].ymax - line[x].ymin; i >= y - line[x].ymin; i--)
			msg[x][i+1] = msg[x][i];
		msg[x][y - line[x].ymin] = 0;
	}

	msg[x][y - line[x].ymin] = ch;
	line[x].ymax++;
	y++;

	for(int i = 0; i <= line[x].ymax - line[x].ymin; i++)
	{
		if(msg[x][i] == 0) continue;
		mvaddch(x, i + line[x].ymin, msg[x][i]);
	}
}

bool CLIController::VimEditor(bool& saveFlag, char* fname)
{
    initscr();
    clear();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	mvprintw(VIM_START_X - 2, VIM_LOG_STY, "%s+", fname);
	for(int i = 0; i < VIM_MAX_Y; i++)
	{
		mvaddch(0, i, '-');
		mvaddch(VIM_START_X - 1, i, '-');
		mvaddch(VIM_LOG_X - 1, i, '-');
	}
	for(int i = VIM_START_X - 1; i <= VIM_LOG_X - 1; i++)
		mvaddch(i, 0, '|');
	mvprintw(VIM_LOG_X + 1, VIM_LOG_STY, "N... >> %s", fname);
    // Show file content
	for (int i = VIM_START_X; i <= xmax; i++)
    {
        for (int j = line[i].ymin; j < line[i].ymax; j++)
        {
            mvaddch(i, j, msg[i][j - 1]);
        }
    }

	x = xmax;
	y = line[x].ymax + 1;
	bool flag = 0;
	int ret;

	bool exit = 0;

	while(true)
	{
		ret = 0;
		ch = getch();
		VimRetkey(ch, ret);
		VimInit(VIM_LOG_X);
		mvprintw(VIM_LOG_X, VIM_LOG_STY, "key %d, ret = %d", ch, ret);
		if(ch == KEY_F(2))
			break;
		else if(ch == ':')
		{
			char save[10] = {0};
			int sp = 0;
			save[sp++] = ch;
			save[sp] = '\0';
			mvaddch(VIM_LOG_X, VIM_ESC_DIS, ch);
			while(true)
			{
				ch = getch();
				if(ch == KEY_ESC) break;

				if(ch != 'w' && ch != 'q' && ch != '!' && ch != '\n')
					continue;
				if(sp > 4) continue;

				if(ch == '\n' && strcmp(save, ":w") == 0)
				{
					move(x, y);
					saveFlag = 1;
					break;
				}
				if(ch == '\n' && strcmp(save, ":wq") == 0)
				{
					exit = 1;
					saveFlag = 1;
					break;
				}
				if(ch == '\n' && strcmp(save, ":q!") == 0)
				{
					exit = 1;
					break;
				}

				save[sp++] = ch;
				save[sp] = '\0';
				for(int i = 0; i < sp; i++)
					mvaddch(VIM_LOG_X, VIM_ESC_DIS + i, save[i]);
			}
			if(exit == 1) break;
		}
		else if(ch == 'i')  //insert mode
		{
			flag = 1;
			mvprintw(VIM_LOG_X + 1, VIM_LOG_STY, "I... >> %s[+]", fname);
			VimInit(VIM_LOG_X);
			mvprintw(VIM_LOG_X, VIM_LOG_STY, "key insert(x: %d, y: %d)", x, y);
			move(x, y);

			while(flag)
			{
				ret = 0;
				ch = getch();
				VimRetkey(ch, ret);

				VimInit(VIM_LOG_X);
				mvprintw(VIM_LOG_X, VIM_LOG_STY, "key %d, ret = %d", ch, ret);
				move(x, y);

				if(ret == 1)
				{
					flag = 0;
					VimInit(VIM_LOG_X + 1);
					mvprintw(VIM_LOG_X + 1, VIM_LOG_STY, "N... >> %s", fname);
					VimInit(VIM_LOG_X);
					mvprintw(VIM_LOG_X, VIM_LOG_STY, "key escape");
					move(x, y);
				}
				if(ret == 2) VimDir();
				if(ret == 3) VimBac();
				if(ret == 4) VimLet();

				move(x, y);
			}
		}
		else if(ch == 27) //esc mode
		{
			flag = 0;
			VimInit(VIM_LOG_X + 1);
			mvprintw(VIM_LOG_X + 1, VIM_LOG_STY, "N... >> %s", fname);
			VimInit(VIM_LOG_X);
			mvprintw(VIM_LOG_X, VIM_LOG_STY, "key escape");
			move(x, y);
		}
		else if(ret == 2)
		{
			VimDir();
			move(x, y);
		}
	}

	endwin();
	return 0;
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
    if(lentmp == 0) return true;
    /*if(lentmp == 0)
        if(!MakeMenu()) return false;*/
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

    if(strcmp(cmd[1], "ls") == 0)   // ls
    {
        if (!uc.CheckR(this->nowiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
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
    if(strcmp(cmd[1], "ll") == 0)   // ll
    {
        if (!uc.CheckR(this->nowiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
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
                    cout << NULL_UNAME << " ";
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
    if(strcmp(cmd[1], "cd") == 0)   // cd <path>
    {
        if(len[2] == 0)
        {
            cmd[2][len[2]++] = '.';
            cmd[2][len[2]++] = '\0';
        }
        if(len[3] != 0) return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if(!(rst.mode & DIRFLAG))
        {
            cout << NOT_DIR << endl;
            return false;
        }
        if (!uc.CheckR(rst, this->uid) || !uc.CheckX(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }

        memcpy((char*)&nowiNode, (char*)&rst, sizeof(iNode));
    }
    if(strcmp(cmd[1], "start") == 0)    // start <processname>
    {
        if(len[2] == 0) return false;
        pid_t newPid;
        if (pic.CreateProcess(cmd[2], this->uid, &newPid))
        {
            cout << "[" << newPid << "] Process " << cmd[2] << " started" << endl;
        }
        else
        {
            cout << DEFAULT_ERROR << "(Too many process)" << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "kill") == 0)    // kill <pid>
    {
        if (len[2] == 0) return false;
        pid_t rstPid = 0;
        if (!GetProcessID(cmd[2], len[2], rstPid))
        {
            cout << INVALID_PID << endl;
            return false;
        }
        if (pic.KillProcess(rstPid))
        {
            cout << "[" << rstPid << "] Process killed" << endl;
        }
        else
        {
            cout << INVALID_PID << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "openr") == 0)    // openr <path> <pid>
    {
        if(len[3] == 0) return false;
        pid_t pid = 0;
        if(!GetProcessID(cmd[3], len[3], pid) || pid >= MAX_PROCESS_CNT)
        {
            cout << INVALID_PID << endl;
            return false;
        }

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (!uc.CheckR(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if(rst.mode & DIRFLAG)
        {
            cout << IS_DIR << endl;
            return false;
        }
        if(this->pic.CheckXlock(rst.bid))
        {
            cout << FILE_LOCKED << endl;
            return false;
        }

        if(!this->pic.FOpen(pid, rst, false))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
        cout << "File opened" << endl;
    }
    if(strcmp(cmd[1], "openw") == 0)    // openw <path> <pid>
    {
        if(len[3] == 0) return false;
        pid_t pid = 0;
        if(!GetProcessID(cmd[3], len[3], pid))
        {
            cout << INVALID_PID << endl;
        }
        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
        {
            cout << INVALID_PATH << endl;
        }
        if(rst.mode & DIRFLAG)
        {
            cout << IS_DIR << endl;
        }
        if(this->pic.CheckXlock(rst.bid))
        {
            cout << FILE_LOCKED << endl;
        }
        if(!this->pic.FOpen(pid, rst, true))
        {
            cout << DEFAULT_ERROR << "(" << INVALID_PID << ")" << endl;
        }
        cout << "File opened" << endl;
    }
    if(strcmp(cmd[1], "close") == 0)    // close <path> <pid>
    {
        if(len[3] == 0) return false;
        pid_t pid = 0;
        if(!GetProcessID(cmd[3], len[3], pid))
        {
            cout << INVALID_PID << endl;
        }
        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
        {
            cout << INVALID_PATH << endl;
        }
        if(rst.mode & DIRFLAG)
        {
            cout << IS_DIR << endl;
        }
        if (!pic.FClose(pid, rst))
        {
            cout << DEFAULT_ERROR << "(" << INVALID_PID << " or"
                << " File is already closed)" << endl;
            return false;
        }
        cout << "File closed" << endl;
    }
    if(strcmp(cmd[1], "mkdir") == 0)    // mkdir <path>
    {
        if(len[2] == 0 || len[3] != 0) return false;

        iNode rst;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], false, &rst))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (!uc.CheckW(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }

        char dirname[FILENAME_MAXLEN];
        int dncnt = 0;
        GetLastSeg(cmd[2], len[2], dirname, dncnt);
        iNode trst;
        if(!fsc.CreateSubDir(rst, dirname, DIR_DEFAULT_FLAG, uid, &trst))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "vim") == 0)  // vim <path>
    {
        if (len[3] != 0) return false;
        // Parse path
        iNode rst, parent;
        bool needTouch = false;
        if (!fsc.ParsePath(nowiNode, cmd[2], false, &parent))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst))
        {
            needTouch = true;
        }
        else
        {
            if(rst.mode & DIRFLAG)
            {
                cout << IS_DIR << endl;
                return false;
            }
        }

        char fname[FILENAME_MAXLEN];
        int flen = 0;
        GetLastSeg(cmd[2], len[2], fname, flen);

        char tmpmsg[VIM_MAX_X][VIM_MAX_Y];
        memset(tmpmsg, 0, sizeof(tmpmsg));
        memset(msg, 0, sizeof(msg));
        memset(line, 0, sizeof(0));

        int cntX = 0;
        if (!needTouch) // open existing file
        {
            if (!uc.CheckR(rst, this->uid))
            {
                cout << ACCESS_DENIED << endl;
                return false;
            }
            if(!this->fsc.GetCutFile(rst, tmpmsg, &cntX))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
        }
        // Init vim params
        xmin = VIM_START_X;
        xmax = (xmin + cntX - 1 < xmin) ? xmin : xmin + cntX - 1;
        if (cntX == 0)
        {
            line[xmin].ymin = 1;
            line[xmin].ymax = 1;
        }
        for(int i = 0; i < cntX; i++)
        {
            strcpy(msg[i + xmin], tmpmsg[i]);
            line[i + xmin].ymin = 1;
            line[i + xmin].ymax = 1 + strlen(msg[i + xmin]);
        }

        bool saveFlag = false;
        VimEditor(saveFlag, fname);

        if(saveFlag)
        {
            if (needTouch)
            {
                if (!uc.CheckW(parent, this->uid))
                {
                    cout << ACCESS_DENIED << endl;
                    return false;
                }
                if (!fsc.Touch(parent, fname, FILE_DEFAULT_FLAG, this->uid, &rst))
                {
                    cout << "Failed to write " << fname << endl;
                    return false;
                }
            }
            else
            {
                if (!uc.CheckW(rst, this->uid))
                {
                    cout << ACCESS_DENIED << endl;
                    return false;
                }
            }

            if(!fsc.SaveCutFile(rst, msg, xmax, line))
            {
                cout << "Failed to write " << fname << endl;
                return false;
            }
        }
    }
    if(strcmp(cmd[1], "rm") == 0)   // rm [-r] <path>
    {
        if(len[2] == 0) return false;
        if((len[3] != 0) && (strcmp(cmd[2], "-r") != 0)) return false;

        iNode rst;
        if(len[3] == 0)
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &rst, false))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            if(rst.mode & DIRFLAG)
            {
                cout << IS_DIR << endl;
                return false;
            }

            if (!uc.CheckW(rst, this->uid))
            {
                cout << ACCESS_DENIED << endl;
                return false;
            }

            if(!this->fsc.DeleteFile(rst))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
        }
        else
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &rst))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            if(!(rst.mode & DIRFLAG))
            {
                cout << NOT_DIR << endl;
                return false;
            }

            if (!uc.CheckW(rst, this->uid))
            {
                cout << ACCESS_DENIED << endl;
                return false;
            }

            if(!this->fsc.DeleteDir(rst))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
        }
    }
    if(strcmp(cmd[1], "chmod") == 0)    // chmod <mode> <path>
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
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (!uc.CheckW(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        mode = ((cmd[2][0]-'0') << 4) | ((cmd[2][1]-'0') << 1);
        if(rst.mode & DIRFLAG) mode = mode | DIRFLAG;
        if(!this->fsc.ChangeMode(rst, (char)mode))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "cp") == 0)   // cp <src> <des>
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode, false))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode, false))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            GetLastSeg(cmd[3], len[3], newname, newlen);
        }
        else
            strcpy(newname, srciNode.name);

        if(!(desiNode.mode & DIRFLAG))
        {
            cout << NOT_DIR << endl;
            return false;
        }
        if (!uc.CheckR(srciNode, this->uid) || !uc.CheckW(desiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if(!this->fsc.Copy(srciNode, desiNode, newname, this->uid))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "mv") == 0)   // mv <src> <des>
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode, false))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode, false))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode, false))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            GetLastSeg(cmd[3], len[3], newname, newlen);
        }
        else
            strcpy(newname, srciNode.name);

        if(!(desiNode.mode & DIRFLAG))
        {
            cout << NOT_DIR << endl;
            return false;
        }
        if (!uc.CheckR(srciNode, this->uid) || !uc.CheckW(desiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if(!this->fsc.Move(srciNode, desiNode, newname))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "touch") == 0)    // touch <path>
    {
        if(len[3] != 0) return false;

        iNode rst;
        if(this->fsc.ParsePath(nowiNode, cmd[2], true, &rst, false))
        {
            cout << FILE_EXISTS << endl;
            return false;
        }
        if(!this->fsc.ParsePath(nowiNode, cmd[2], false, &rst, false))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (!uc.CheckW(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }

        char newname[FILENAME_MAXLEN];
        int newlen = 0;
        GetLastSeg(cmd[2], len[2], newname, newlen);
        iNode newrst;
        if(!this->fsc.Touch(rst, newname, FILE_DEFAULT_FLAG, uid, &newrst))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "lnh") == 0)  // lnh <srcfile> <des>
    {
        if(len[3] == 0) return false;

        iNode srciNode, desiNode;
        if(!this->fsc.ParsePath(nowiNode, cmd[2], true, &srciNode, false))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        char linkname[FILENAME_MAXLEN];
        int linklen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            GetLastSeg(cmd[3], len[3], linkname, linklen);
        }
        else strcpy(linkname, srciNode.name);

        if (!uc.CheckR(srciNode, this->uid) || !uc.CheckW(desiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }

        if(!this->fsc.LinkH(srciNode, desiNode, linkname))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "lns") == 0)  // lns <src> <des>
    {
        if(len[3] == 0) return false;

        iNode desiNode;
        char linkname[FILENAME_MAXLEN];
        int linklen = 0;
        if(!this->fsc.ParsePath(nowiNode, cmd[3], true, &desiNode))
        {
            if(!this->fsc.ParsePath(nowiNode, cmd[3], false, &desiNode))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
        }
        GetLastSeg(cmd[3], len[3], linkname, linklen);

        if (!uc.CheckW(desiNode, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }

        if(!this->fsc.LinkS(cmd[2], desiNode, linkname, this->uid))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
    }
    if(strcmp(cmd[1], "useradd") == 0)  // useradd <username> <password> (root required)
    {
        if (!uc.CheckRoot(this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if(len[3] == 0) return false;
        // Check if user exists
        uid_t tu;
        if (uc.GetUidByUsername(cmd[2], &tu))
        {
            cout << "User " << cmd[2] << " already exists" << endl;
            return false;
        }
        // Add user
        if (uc.AddUser(cmd[2], cmd[3], true))
        {
            cout << "User " << cmd[2] << " added" << endl;
            return true;
        }
        else
        {
            cout << "Failed to add user " << cmd[2] << endl;
            return false;
        }
    }
    if (strcmp(cmd[1], "userdel") == 0) // userdel [-r] <username> (root required)
    {
        if (!uc.CheckRoot(this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if(len[2] == 0) return false;
        if((len[3] != 0) && (strcmp(cmd[2], "-r") != 0)) return false;
        if (strcmp(cmd[2], "-r") == 0)  // userdel -r <username>
        {
            uid_t tu;
            if (uc.GetUidByUsername(cmd[3], &tu))
            {
                if (uc.DelUser(tu, true))
                {
                    cout << "User " << cmd[3] << " deleted" << endl;
                    return true;
                }
                else
                {
                    cout << "Failed to delete user " << cmd[3] << endl;
                    return false;
                }
            }
            else
            {
                cout << "User " << cmd[3] << " not exists" << endl;
                return false;
            }
        }
        else    // userdel <username>
        {
            uid_t tu;
            if (uc.GetUidByUsername(cmd[2], &tu))
            {
                if (uc.DelUser(tu, false))
                {
                    cout << "User " << cmd[2] << " deleted" << endl;
                    return true;
                }
                else
                {
                    cout << "Failed to delete user " << cmd[2] << endl;
                    return false;
                }
            }
            else
            {
                cout << "User " << cmd[2] << " not exists" << endl;
                return false;
            }
        }
    }
    if (strcmp(cmd[1], "passwd") == 0) // passwd <oldpwd> <newpwd>
    {
        if (len[2] == 0) return false;
        if (!this->uc.CheckPwd(this->uid, cmd[2]))
        {
            cout << "Invalid old password" << endl;
            return false;
        }
        if (!this->uc.ChangePwd(this->uid, cmd[3]))
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
        cout << "Password changed" << endl;
    }
    if (strcmp(cmd[1], "su") == 0) // su
    {
        return this->Login();
    }
    if (strcmp(cmd[1], "format") == 0) // format (root required)
    {
        if (!uc.CheckRoot(this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if (!fsc.Format())
        {
            cout << DEFAULT_ERROR << endl;
            return false;
        }
        // Formatted, need restart
        cout << "Formatted. Please restart the system" << endl;
        exitFlag = true;
    }
    if (strcmp(cmd[1], "echo") == 0) // echo <path> <content>
    {
        if (len[2] == 0 || len[3] == 0) return false;
        iNode rst;
        if (this->fsc.ParsePath(nowiNode, cmd[2], true, &rst, false))
        {   // Append to existing file
            if (rst.mode & DIRFLAG)
            {
                cout << IS_DIR << endl;
                return false;
            }
            if (!uc.CheckW(rst, this->uid))
            {
                cout << ACCESS_DENIED << endl;
                return false;
            }
            if (!fsc.WriteFileFromBuf(rst, rst.size, len[3], cmd[3]))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
            cout << DEFAULT_SUCCESS << endl;
        }
        else
        {   // Write to new file
            if(!this->fsc.ParsePath(nowiNode, cmd[2], false, &rst, false))
            {
                cout << INVALID_PATH << endl;
                return false;
            }
            if (!uc.CheckW(rst, this->uid))
            {
                cout << ACCESS_DENIED << endl;
                return false;
            }
            char newname[FILENAME_MAXLEN];
            int newlen = 0;
            GetLastSeg(cmd[2], len[2], newname, newlen);
            iNode newrst;
            if(!this->fsc.Touch(rst, newname, FILE_DEFAULT_FLAG, uid, &newrst))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
            if (!this->fsc.WriteFileFromBuf(newrst, 0, len[3], cmd[3]))
            {
                cout << DEFAULT_ERROR << endl;
                return false;
            }
            cout << DEFAULT_SUCCESS << endl;
        }
    }
    if (strcmp(cmd[1], "cat") == 0) // cat <path>
    {
        if (len[2] == 0) return false;
        iNode rst;
        if (!fsc.ParsePath(nowiNode, cmd[2], true, &rst, false))
        {
            cout << INVALID_PATH << endl;
            return false;
        }
        if (rst.mode & DIRFLAG)
        {
            cout << IS_DIR << endl;
            return false;
        }
        if (!uc.CheckR(rst, this->uid))
        {
            cout << ACCESS_DENIED << endl;
            return false;
        }
        if (rst.size != 0)
        {
            char* buf = new char[rst.size + 10];
            if (!fsc.ReadFileToBuf(rst, 0, rst.size, buf))
            {
                delete[] buf;
                cout << DEFAULT_ERROR << endl;
                return false;
            }
            for (unsigned int i = 0; i < rst.size; i++)
                cout << buf[i];
            cout << endl;
            delete[] buf;
        }
    }
    if (strcmp(cmd[1], "top") == 0) // top
    {
        this->pic.PrintStatus();
    }
    if (strcmp(cmd[1], "clear") == 0) // clear
    {
        cout << CLEAR_LINUX;
    }
    if(strcmp(cmd[1], "exit") == 0) // exit
    {
        cout << BYE << endl;
        exitFlag = true;
        return true;
    }

    return true;
}

bool CLIController::Login()
{
    char username[MAX_UNAME_LEN];
    char password[MAX_PWD_LEN];
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    uid_t uid;
    cout << "Username: ";
    cin >> username;
    cin.ignore(100, '\n');
    if (!uc.GetUidByUsername(username, &uid))
    {
        cout << "Invalid username" << endl;
        return false;
    }
    else
    {
        cout << "Password: ";
        cin >> password;
        cin.ignore(100, '\n');
        if (!uc.CheckPwd(uid, password))
        {
            cout << "Invalid password" << endl;
            return false;
        }
    }
    // Clear
    cout << CLEAR_LINUX;
    // Set now uid
    this->uid = uid;
    // Set now path to /home/<username>
    char path[MAX_CMD_LEN];
    memset(path, 0, sizeof(path));
    strcpy(path, HOMEDIR_ABSPATH);
    strcpy(path + strlen(HOMEDIR_ABSPATH), "/");
    strcpy(path + strlen(HOMEDIR_ABSPATH) + 1, username);
    iNode homeiNode, fooiNode;
    if (fsc.ParsePath(fooiNode, path, true, &homeiNode))
        memcpy((char*)&this->nowiNode, (char*)&homeiNode, sizeof(iNode));
    else
        cout << "Failed to cd " << path << ", use / instead" << endl;
    return true;
}
