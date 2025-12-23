
class IYgoMasterMgr 
{
public:
    virtual void Run() = 0;
};

void GetYgoMasterMgr(IYgoMasterMgr** imp);