#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <map>
#include <vector>
#include <mutex>

struct MapOp {
    int opcode; // operation code: 1- update value
    int arg1;   // customer_id to apply the operation
    int arg2;   // parameter for the operation
};

class CustRecord {
private:
    std::map<int, int> record_map;
    mutable std::mutex record_mutex;
public:
    void SetCustRecord(int customer_id, int last_order_number);
    int GetCustRecord(int customer_id);
};

class StateMachineReplicationLog {
private:
    std::vector<MapOp> log_data;
    mutable std::mutex log_mutex;
public:
    void AppendingOperation(const MapOp& op);
    MapOp FetchLog(int index);
};

class StateMachine {
private:
    CustRecord customer_record;
    StateMachineReplicationLog smr_log;
    int latest_index_stored;
public:
    StateMachine();
    void AppendingOperation(const MapOp& op);
    void ApplyOperation(const MapOp& op);

    int GetCustomerRecord(int customer_id);
    int GetLastCommittedLogIndex();
    MapOp FetchLog(int index);
};


#endif // STATEMACHINE_H