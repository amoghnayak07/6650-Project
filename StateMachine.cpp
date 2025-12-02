#include "StateMachine.h"

void CustRecord::SetCustRecord(int customer_id, int last_order_number)
{
    std::lock_guard<std::mutex> lg(record_mutex);
    record_map[customer_id] = last_order_number;
}

int CustRecord::GetCustRecord(int customer_id)
{
    std::lock_guard<std::mutex> lg(record_mutex);
    auto it = record_map.find(customer_id);
    if (it != record_map.end())
    {
        return it->second;
    }
    return -1;
}

void StateMachineReplicationLog::AppendingOperation(const MapOp &op)
{
    std::lock_guard<std::mutex> lg(log_mutex);
    log_data.push_back(op);
}
MapOp StateMachineReplicationLog::FetchLog(int index)
{
    std::lock_guard<std::mutex> lg(log_mutex);
    if (index >= 0 && index < (int)log_data.size())
    {
        return log_data[index];
    }
    return MapOp{-1, -1, -1};
}

StateMachine::StateMachine()
{
    latest_index_stored = -1;
}

void StateMachine::AppendingOperation(const MapOp &op)
{
    smr_log.AppendingOperation(op);
}

void StateMachine::ApplyOperation(const MapOp &op)
{
    if (op.opcode == 1)
    {
        customer_record.SetCustRecord(op.arg1, op.arg2);
    }
    latest_index_stored++;
}

int StateMachine::GetCustomerRecord(int customer_id)
{
    return customer_record.GetCustRecord(customer_id);
}

int StateMachine::GetLastCommittedLogIndex()
{
    return latest_index_stored;
}

MapOp StateMachine::FetchLog(int index)
{
    return smr_log.FetchLog(index);
}

void StateMachine::ApplyUpTo(int index)
{
    for (int i = latest_index_stored + 1; i <= index; i++)
    {
        MapOp op = smr_log.FetchLog(i);
        if (op.opcode == -1)
        {
            break;
        }
        ApplyOperation(op);
    }
}

int StateMachine::GetLogSize()
{
    return GetLastCommittedLogIndex() + 1;
}