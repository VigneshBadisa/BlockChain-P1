#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "helper.h"

#include <unordered_set>
#include <unordered_map>
#include <set>


class Node;

struct simulator {

public :

    int numNodes;
    float z0,z1;                                        // percentage of slow nodes, low hash nodes
    ld T_tx, Tk;                                        // T_tx, Tk
    ld simclock = 0;


    std::unordered_map<int,Node*> nodes;
    std::unordered_map<int,std::unordered_set<int>> adj_nodes;
    std::set<Event*,EventComparator> event_queue;

    Block* GENESIS_blk;

    simulator(int numNodes_, float z0_ , float z1_ , ld T_tx_, ld Tk_ ):
        numNodes(numNodes_), z0(z0_), z1(z1_), T_tx(T_tx_), Tk(Tk_) {}

    void start();
    void create_network();
    void add_event();
    void execute_top_event();
    void write_tree_file();         // TBD

    // Visualization                // TBD

    ~simulator() = default;
};

#endif