#include<cstring>
#include "UserController.h"
#include "BSFSParams.h"

using namespace std;

UserController::UserController()
{
    //ctor
}

bool UserController::GetUsernameByUid(int uid, char* username)
{
    // TODO
    if(uid == ROOT_UID)
        strcpy(username, "root");
    return true;
}
