#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "helper.h"

#include <unordered_set>
#include <unordered_map>

class Node;

struct simulator {

public :

    int numNodes;
    float z0,z1;                                        // percentage of slow nodes, low hash nodes
    ld T_tx, Tk;                                        // T_tx, Tk


    std::unordered_map<int,Node*> nodes;
    std::unordered_map<int,std::vector<int>> adj_nodes;


    Block* GENESIS_blk;

    simulator(int numNodes_, float z0_ , float z1_ , ld T_tx_, ld Tk_ ):
        numNodes(numNodes_), z0(z0_), z1(z1_), T_tx(T_tx_), Tk(Tk_) {}

    void start();
    void create_network();
    void create_genesis_blk();
    void add_event();
    void execute_top_event();
    void write_tree_file();

    ~simulator();
};

#endif