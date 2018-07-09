#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include <cstring>
#include "BSFSParams.h"

struct User
{
    char username[MAX_UNAME_LEN];
    char password[MAX_PWD_LEN];
    bool valid; // flag to identify whether the user data is valid

    User()
    {
        memset((char*)this->username, 0, sizeof(User));
    }

    User(const User& u)
    {
        memcpy((char*)this->username, (char*)u.username, sizeof(User));
    }
};

#endif // USER_H_INCLUDED
