#ifndef __PRIMARYSTUB_H__
#define __PRIMARYSTUB_H__

#include "ClientSocket.h"
#include "Messages.h"
#include <memory>

class PrimaryServerStub
{
    private:
        std::unique_ptr<ClientSocket> socket;

    public:
        PrimaryServerStub();
        int Init(std::string ip, int port);
        LatestState GetState();
        ReplicationResponse Replicate(ReplicationRequest request);
};

#endif // end of #ifndef __PRIMARYSTUB_H__