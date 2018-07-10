#ifndef USERCONTROLLER_H
#define USERCONTROLLER_H

#include "FSController.h"
#include "User.h"
#include <vector>

using namespace std;

class UserController
{
    public:
        UserController(FSController& _fsc);
        bool InitLoadUserFile();
        bool GetUsernameByUid(const uid_t& uid, char* username);
        bool GetUidByUsername(const char* username, uid_t *uid);
        bool CheckPwd(const uid_t& uid, const char* pwd);
        bool ChangePwd(const uid_t& uid, const char* pwd);
        bool AddUser(const char* username, const char* password, bool createHome = true);
        bool DelUser(const uid_t& uid, bool removeHome = false);
        bool CheckRoot(const uid_t& uid);

    protected:

    private:
        FSController& fsc;
        vector<User> userList;
        iNode ufiNode;
        bool DistributeUid(uid_t* uid);

};

#endif // USERCONTROLLER_H
