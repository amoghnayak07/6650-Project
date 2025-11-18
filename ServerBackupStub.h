#ifndef __BACKUPSTUB_H__
#define __BACKUPSTUB_H__

#include "ServerSocket.h"
#include "Messages.h"
#include <memory>

class BackupServerStub
{
    private:
        std::unique_ptr<ServerSocket> socket;
    public:
        BackupServerStub();
        void Init(std::unique_ptr<ServerSocket> socket);
        void SendReplicationInfo(ReplicationResponse response);
        ReplicationRequest ReceiveReplicationInfo();
        int SendLatestState(LatestState state);
};

#endif // end of #ifndef __BACKUPSTUB_H__