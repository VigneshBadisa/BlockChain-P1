#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "helper.h"
#include "node.h"
#include <unordered_map>

struct simulator {

public :

    int no_of_peers = 10;
    float z0,z1; 
    unordered_map<int,Node> nodes;
    unordered_map<int,vector<int>> adj_nodes;
    float T_tx;    //T_tx
    float queue_delay_constant = 0.09375;               // queing delay between two nodes 
    float qij_min = 0.010,qij_max = 0.500 ;             // speed of light delay
    float cij_min = 5,cij_max = 100;                    // link speed between two nodes

    Block GENESIS_blk;
    Eventqueue Q;

    void create_network();
    void create_genesis_blk();
    void add_event();
    void execute_top_event();
    void write_tree_file();

    ~simulator();
};

#endif