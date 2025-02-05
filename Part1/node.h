#ifndef NODE_H
#define NODE_H

#include "helper.h"

#include <random>
#include <unordered_map>
#include <utility>

struct simulator;

class Node {

public:

    int id;
    bool is_slow;                // 1 - Slow , 0 - Fast
    ld Ttx;                      // Mean Interval time for creating transactions
    
    int numMinedblks = 0;                       // No of blks mined by the Node/peer
    ld hash_power;                              // Hash Power of the Node

    simulator* simul;

    std::unordered_map<int,std::pair<int,int>> latency;          // Latency info 

    Block* genesis_blk;             
    Block* tail_blk;                            // Tail blk of the longest chain at any point of time
    Block* mining_blk;                          // Block being mined by the Node/Peer       

    std::vector<int> expired_txns;                   // Txns that are added to the blockchain
    std::vector<int> txn_pool;                       // Transactions waiting to be added to the block chain
    std::vector<int> blockchain;                     //  Blocks of longest chain
    std::vector<int> orphan_blks;                    //  blocks invalidated from the longest chain or blocks of a stale branch 

    std::unordered_map<int, Txn*> Alltxns;           
    std::unordered_map<int, Block*> AllBlks;  
    std::unordered_map<int,int> wallet;              // Balance amount of the peers over the blockchain

    std::exponential_distribution<ld> generate_Ttx, generate_tk;
    std::uniform_int_distribution<int> select_peer;


    Node(int id_, bool is_slow_ , ld Ttx_, ld hash_power_ , simulator* simul_, Block* genesis_blk_);
    ~Node();

    void get_balance();
    void update_wallet();
    bool is_txn_valid();
    void create_txn();
    void send_txn();
    void recv_txn();
    void is_blk_valid();
    void create_blk();
    void send_blk();
    void recv_blk();

};

#endif