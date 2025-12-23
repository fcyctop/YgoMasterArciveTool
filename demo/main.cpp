#include<iostream>
#include<interface.h>
using namespace std;

int main()
{
    IYgoMasterMgr* mgr;
    GetYgoMasterMgr(&mgr);
    mgr->Run();
    delete mgr;
    return 0;
}
