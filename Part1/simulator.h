#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "helper.h"

#include <unordered_set>
#include <unordered_map>
#include <set>

#define MAX_BLK_SIZE 1000
#define CAPITAL 20

class Node;

struct simulator {

public :

    int numNodes;
    float z0,z1;                                        // percentage of slow nodes, low hash nodes
    ld T_tx, Tk;                                        // T_tx, Tk
    ld simclock = 0;
    ld simEndtime;

    int total_mined_blks = 0;

    std::unordered_map<int,Node*> nodes;
    std::unordered_map<int,std::unordered_set<int>> adj_nodes;
    std::set<Event*,EventComparator> event_queue;

    Block* GENESIS_blk;
    Event* top_event;

    simulator(int numNodes_, float z0_ , float z1_ ,ld simEndtime_, ld T_tx_, ld Tk_ ):
        numNodes(numNodes_), z0(z0_), z1(z1_),T_tx(T_tx_), Tk(Tk_),simEndtime(simEndtime_) {}

    void start();
    void create_network();
    void add_event(Event* E);
    void delete_event(Event* E);
    void execute_event(Event* E,bool stop_create_events);
    void write_tree_file();         // TBD

    // Visualization                // TBD

    ~simulator() = default;
};

#endif