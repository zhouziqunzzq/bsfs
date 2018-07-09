#include<cstring>
#include "UserController.h"

using namespace std;

UserController::UserController()
{
    //ctor
}

bool UserController::GetUsernameByUid(int uid, char* username)
{
    // TODO
    if(uid == 0)
        strcpy(username, "root");
    return true;
}
