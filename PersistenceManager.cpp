#include "PersistenceManager.h"
#include <fstream>
#include <iostream>

PersistenceManager::PersistenceManager(int fact_id)
    : factory_id(fact_id) {
    filename = "wal_factory_" + std::to_string(factory_id) + ".wal";
}

bool PersistenceManager::Open() {
#ifdef NO_PERSISTENCE
    // Persistence disabled at compile time: act as if WAL is ready
    (void)filename;
    return true;
#else
    std::ofstream f(filename, std::ios::binary | std::ios::app);
    if (!f.is_open()) {
        std::cerr << "Failed to open WAL file: " << filename << std::endl;
        return false;
    }
    return true;
#endif
}

bool PersistenceManager::AppendEntry(int index, const MapOp &op) {
#ifdef NO_PERSISTENCE
    // No-op when persistence disabled
    (void)index; (void)op;
    return true;
#else
    std::ofstream f(filename, std::ios::binary | std::ios::app);
    if (!f.is_open()) {
        std::cerr << "Append failed: cannot open WAL file" << std::endl;
        return false;
    }

    int32_t i_idx = index;
    int32_t i_op  = op.opcode;
    int32_t a1    = op.arg1;
    int32_t a2    = op.arg2;

    f.write(reinterpret_cast<char*>(&i_idx), sizeof(int32_t));
    f.write(reinterpret_cast<char*>(&i_op), sizeof(int32_t));
    f.write(reinterpret_cast<char*>(&a1), sizeof(int32_t));
    f.write(reinterpret_cast<char*>(&a2), sizeof(int32_t));

    if (!f.good()) {
        std::cerr << "Append failed" << std::endl;
        return false;
    }

    f.flush();

    return true;
#endif
}

bool PersistenceManager::LoadAll(std::vector<MapOp> &out_log) {
#ifdef NO_PERSISTENCE
    // Persistence disabled: nothing to load
    (void)out_log;
    return true;
#else
    std::ifstream f(filename, std::ios::binary);
    if (!f.is_open()) {
        return true;
    }

    while (true) {
        int32_t idx;
        int32_t opcode;
        int32_t arg1;
        int32_t arg2;

        f.read(reinterpret_cast<char*>(&idx), sizeof(int32_t));
        if (!f.good()) break;

        f.read(reinterpret_cast<char*>(&opcode), sizeof(int32_t));
        f.read(reinterpret_cast<char*>(&arg1), sizeof(int32_t));
        f.read(reinterpret_cast<char*>(&arg2), sizeof(int32_t));

        if (!f.good()) break;

        MapOp op;
        op.opcode = opcode;
        op.arg1 = arg1;
        op.arg2 = arg2;

        out_log.push_back(op);
    }
    return true;
#endif
}