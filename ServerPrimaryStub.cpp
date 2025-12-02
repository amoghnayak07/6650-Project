#include "ServerPrimaryStub.h"

PrimaryServerStub::PrimaryServerStub() {}

int PrimaryServerStub::Init(std::string ip, int port)
{
    socket = std::unique_ptr<ClientSocket>(new ClientSocket());
    if (socket->Init(ip, port) == 0) {
        return 0; // Connection failed
    }
    RoleIdentifier r_id;

    r_id.SetRole(RoleIdentifier::ADMIN);
    char buffer[r_id.Size()];
    r_id.Marshal(buffer);
    int size = r_id.Size();

    if (socket->Send(buffer, size, 0) == 0) {
        return 0;
    }
    return 1;
}

LatestState PrimaryServerStub::GetState()
{
    LatestState state;   
    char buffer[state.Size()];
    int size = state.Size();

    if (socket->Recv(buffer, size, 0) == 0) {
        state.SetState(-2, -2); // Failure
        return state;
    }

    state.Unmarshal(buffer);

    return state;
}

ReplicationResponse PrimaryServerStub::Replicate(ReplicationRequest request)
{
    ReplicationResponse replication_response;   
    char buffer[request.Size()];
    int size;

    request.Marshal(buffer);
    size = request.Size();
    if (socket->Send(buffer, size, 0) == 0) {
        replication_response.SetResponse(false);
        return replication_response;
    }

    size = replication_response.Size();
    if (socket->Recv(buffer, size, 0) == 0) {
        replication_response.SetResponse(false);
        return replication_response;
    }
    replication_response.Unmarshal(buffer);

    return replication_response;
}
