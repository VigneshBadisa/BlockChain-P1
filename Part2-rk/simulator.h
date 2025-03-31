#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "helper.h"

#include <unordered_set>
#include <unordered_map>
#include <set>

#define MAX_BLK_SIZE 1000
#define CAPITAL 20
#define START_MINING_TIME 5
#define MAX_TRANSACTIONS 120000
#define MAX_BLOCKS 110

class Node;

struct simulator {

public :

    int numNodes;
    float m;
    ld Tt, T_tx, Tk;                                        // Tt, T_tx, Tk
    ld simclock = 0;
    ld simEndtime;

    bool eclipse_attack;

    int total_transactions = 0;
    int total_mined_blks = 0;
    int ringmaster;

    std::unordered_map<int,Node*> nodes;
    std::unordered_map<int,int> malnodes;
    std::unordered_map<int,std::unordered_set<int>> adj_nodes;
    std::unordered_map<int,std::unordered_set<int>> adj_malnodes;
    std::set<Event*,EventComparator> event_queue;

    Block* GENESIS_blk;
    Event* top_event;


    simulator(int numNodes_, float m_,ld simEndtime_,ld Tt_, ld T_tx_, ld Tk_ );

    void start();
    void create_network();
    void create_malicious_network();
    void add_event(Event* E);
    void delete_event(Event* E);
    void execute_event(Event* E,bool stop_create_events);
    void write_tree_file();         // TBD

    // Visualization                // TBD

    ~simulator() = default;
};

#endif