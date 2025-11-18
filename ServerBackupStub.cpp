#include "ServerBackupStub.h"

#include <iostream>

BackupServerStub::BackupServerStub() {}

void BackupServerStub::Init(std::unique_ptr<ServerSocket> socket)
{
    this->socket = std::move(socket);
}

void BackupServerStub::SendReplicationInfo(ReplicationResponse response)
{
    char buffer[response.Size()];
    int size;

    response.Marshal(buffer);
    size = response.Size();
    socket->Send(buffer, size, 0);
}

ReplicationRequest BackupServerStub::ReceiveReplicationInfo()
{
    ReplicationRequest request;
    char buffer[request.Size()];
    int size = request.Size();
    
    while (size > 0) {
        if (socket->Recv(buffer, size, 0) <= 0) {
            request.SetMessage(-1, -1, -1, MapOp());
            return request;
        }
        size -= size;
    }

    request.Unmarshal(buffer);

    return request;
}

int BackupServerStub::SendLatestState(LatestState state)
{
    char buffer[state.Size()];
    int size;

    state.Marshal(buffer);
    size = state.Size();
    if (socket->Send(buffer, size, 0) == 0) {
        return 0;
    }
    return 1;
}