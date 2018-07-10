#include <cstring>
#include <iostream>
#include <string>
#include "UserController.h"
#include "BSFSParams.h"
#include "iNode.h"

using namespace std;

UserController::UserController(FSController& _fsc) : fsc(_fsc)
{
    return;
}

bool UserController::DistributeUid(uid_t* uid)
{
    uid_t u = 0;
    for (auto i = userList.begin(); i != userList.end(); i++)
    {
        u++;
        // Distribute first spare uid
        if (!i->valid)
        {
            *uid = u;
            return true;
        }
    }
    u++;
    // Not enough spare space, append file and distribute uid
    User tu;
    if (!fsc.WriteFileFromBuf(ufiNode, ufiNode.size, sizeof(User), (char*)&tu))
        return false;
    userList.push_back(tu);
    *uid = u;
    return true;
}

bool UserController::AddUser(const char* username, const char* password, bool createHome)
{
    User u;
    strcpy(u.username, username);
    strcpy(u.password, password);
    u.valid = true;
    // Distribute uid
    uid_t newUid;
    if (DistributeUid(&newUid)) // Edit file
    {
        // Save to file
        if (!fsc.WriteFileFromBuf(this->ufiNode, (newUid - 1) * sizeof(User),
            sizeof(User), (char*)&u))
            return false;
        // Update vector
        memcpy((char*)&userList[newUid - 1], (char*)&u, sizeof(User));
    }
    else    // Append file
    {
        // Save to file
        if (!fsc.WriteFileFromBuf(this->ufiNode, this->ufiNode.size,
            sizeof(User), (char*)&u))
            return false;
        // Update vector
        userList.push_back(u);
    }
    if (createHome)    // Create /home/<username>
    {
        char uname[MAX_UNAME_LEN], homepath[MAX_CMD_LEN] = HOMEDIR_ABSPATH;
        memset(uname, 0, sizeof(uname));
        strcpy(uname, username);
        iNode homeiNode, fooiNode, rstiNode;
        if (!fsc.ParsePath(fooiNode, homepath, true, &homeiNode))
            return false;
        if (!fsc.CreateSubDir(homeiNode, uname, DIR_DEFAULT_FLAG, newUid, &rstiNode))
            return false;
    }
    return true;
}

bool UserController::InitLoadUserFile()
{
    // Open userfile iNode
    char path[FILENAME_MAXLEN] = USER_FILE_FULLPATH;
    iNode fooiNode;
    if (!fsc.ParsePath(fooiNode, path, true, &this->ufiNode))
        return false;
    // Init if necessary
    if (this->ufiNode.size == 0)
    {
        if (!this->AddUser(ROOT_USERNAME, ROOT_DEFAULT_PWD, true))
            return false;
        return true;
    }
    // Load userfile to vector
    User* ua = new User[this->ufiNode.size / sizeof(User)];
    if (!fsc.ReadFileToBuf(this->ufiNode, 0, this->ufiNode.size, (char*)ua))
    {
        delete[] ua;
        return false;
    }
    for (int i = 0; i < (int)(ufiNode.size / sizeof(User)); i++)
        this->userList.push_back(ua[i]);
    delete[] ua;
    return true;
}

bool UserController::GetUsernameByUid(const uid_t& uid, char* username)
{
    if (uid > userList.size())
        return false;
    else
    {
        if (!userList[uid - 1].valid) return false;
        strcpy(username, userList[uid - 1].username);
        return true;
    }
}

bool UserController::GetUidByUsername(const char* username, uid_t *uid)
{
    uid_t u = 1;
    for (auto i = userList.begin(); i != userList.end(); i++)
    {
        if (i->valid && (strcmp(i->username, username) == 0))
        {
            *uid = u;
            return true;
        }
        u++;
    }
    return false;
}

bool UserController::CheckPwd(const uid_t& uid, const char* pwd)
{
    if (uid > userList.size())
        return false;
    else
    {
        if (strcmp(userList[uid - 1].password, pwd) == 0)
            return true;
        else
            return false;
    }
}

bool UserController::ChangePwd(const uid_t& uid, const char* pwd)
{
    if (uid > userList.size())
        return false;
    else
    {
        User tu;
        memcpy((char*)&tu, (char*)&userList[uid - 1], sizeof(User));
        memset(tu.password, 0, MAX_PWD_LEN);
        strcpy(tu.password, pwd);
        // Update user file
        if (!fsc.WriteFileFromBuf(ufiNode, (uid - 1) * sizeof(User),
            sizeof(User), (char*)&tu))
            return false;
        // Update user list
        strcpy(userList[uid - 1].password, pwd);
        return true;
    }
}

bool UserController::DelUser(const uid_t& uid, bool removeHome)
{
    if (uid > userList.size())
        return false;

    char homepath[MAX_CMD_LEN];
    memset(homepath, 0, sizeof(homepath));
    strcpy(homepath, HOMEDIR_ABSPATH);
    strcpy(homepath + strlen(HOMEDIR_ABSPATH), "/");
    strcpy(homepath + strlen(HOMEDIR_ABSPATH) + 1, userList[uid - 1].username);

    User u;
    if (!fsc.WriteFileFromBuf(this->ufiNode, (uid - 1) * sizeof(User),
        sizeof(User), (char*)&u))
        return false;
    memcpy((char*)&userList[uid - 1], (char*)&u, sizeof(User));

    if (removeHome) // delete /home/<username>
    {
        iNode homeiNode, fooiNode;
        if (!fsc.ParsePath(fooiNode, homepath, true, &homeiNode))
            return false;
        if (!fsc.DeleteDir(homeiNode))
            return false;
    }

    return true;
}

bool UserController::CheckRoot(const uid_t& uid)
{
    return uid == ROOT_UID;
}
