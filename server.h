#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <string>

using namespace std;

typedef enum SERVER_STATE_tag
{
    WAITING = 0,
    PROCESS_LS = 1,
    PROCESS_SEND = 2,
    PROCESS_GET = 3,
    PROCESS_REMOVE = 4,
    SHUTDOWN = 5
}Server_State_T;

bool checkFile(const char *fileName);
int checkDirectory (string dir);
int getdir (string dir, vector<string> &files);
#endif
