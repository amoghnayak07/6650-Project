#ifndef PERSISTENCE_MANAGER_H
#define PERSISTENCE_MANAGER_H

#include <string>
#include <vector>
#include "StateMachine.h"

class PersistenceManager {
    public:
        explicit PersistenceManager(int factory_id);

        // create WAL
        bool Open();

        // append an entry to WAL
        bool AppendEntry(int index, const MapOp &op);

        // load all the entries from WAL during startup
        bool LoadAll(std::vector<MapOp> &out_log);

    private:
        int factory_id;
        std::string filename;
};

#endif