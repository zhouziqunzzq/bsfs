#include <cstring>
#include <iostream>
#include "UserController.h"
#include "BSFSParams.h"

using namespace std;

UserController::UserController(FSController& _fsc) : fsc(_fsc)
{
    return;
}

bool UserController::DistributeUid(uid_t* uid)
{
    uid_t u = 1;
    for (auto i = userList.begin(); i != userList.end(); i++)
    {
        // Distribute first spare uid
        if (!i->valid)
        {
            *uid = u;
            return true;
        }
        u++;
    }
    return false;
}

bool UserController::AddUser(const char* username, const char* password)
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
        if (!this->AddUser(ROOT_USERNAME, ROOT_DEFAULT_PWD))
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

bool UserController::DelUser(const uid_t& uid)
{
    User u;
    if (!fsc.WriteFileFromBuf(this->ufiNode, (uid - 1) * sizeof(User),
        sizeof(User), (char*)&u))
        return false;
    memcpy((char*)&userList[uid - 1], (char*)&u, sizeof(User));
    return true;
}
