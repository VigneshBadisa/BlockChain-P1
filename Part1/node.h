#ifndef NODE_H
#define NODE_H

#include "helper.h"

#include <random>
#include <unordered_map>
#include <utility>
#include <set>
#include <unordered_set>

struct simulator;

class Node {

public:

    int id;
    bool is_slow;                // 1 - Slow , 0 - Fast
    ld Ttx;                      // Mean Interval time for creating transactions
    
    int numMinedblks = 0;                       // No of blks mined by the Node/peer
    ld hash_power;                              // Hash Power of the Node


    std::vector<int> adj_peers;
    std::unordered_map<int,Link*> latency;          // Latency info 

    Block* genesis_blk;             
    Block* tail_blk;                            // Tail blk of the longest chain at any point of time
    Block* mining_blk;                          // Block being mined by the Node/Peer       

    std::unordered_set<int> Alltxns;                   // Txns that are received till now
    std::set<Txn*, Txncomparator> txn_pool;                       // Transactions waiting to be added to the block chain
    std::vector<int> blockchain;                     //  Blocks of longest chain
    std::vector<int> orphan_blks;                    //  blocks invalidated from the longest chain or blocks of a stale branch 

    std::unordered_map<int, Block*> AllBlks;  
    std::unordered_map<int,int> wallet;              // Balance amount of the peers over the blockchain

    std::exponential_distribution<ld> generate_Ttx, generate_tk;
    std::uniform_int_distribution<int> select_payee;
    std::uniform_real_distribution<ld> select_rand_real;

    Node(int id_, bool is_slow_ , ld Ttx_, ld hash_power_ , Block* genesis_blk_);
    ~Node() = default;

    void get_balance();             // TBD
    void update_wallet();           
    bool is_txn_valid(Txn* T);            // TBD
    void create_txn(simulator* simul,bool stop_create_events);
    void mine_new_blk();
    void send_txn(int peer_id,Txn* T, simulator* simul);                // TBD
    bool recv_txn(int peer_id,Txn* T, simulator* simul);               
    void is_blk_valid(Block * B);            // TBD
    void create_blk();
    void send_blk();                // TBD
    void recv_blk();

};

std::ostream& operator<<(std::ostream& os, const Node& node);

#endif