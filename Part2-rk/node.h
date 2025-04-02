#ifndef NODE_H
#define NODE_H

#include "helper.h"

#include <random>
#include <unordered_map>
#include <utility>
#include <set>
#include <unordered_set>


#define MINING_FEE 50

struct simulator;

class Node {

public:

    int id;
    bool is_slow, is_highhash;                // 1 - Slow , 0 - Fast
    ld Tt, Ttx, Tk;                      // Mean Interval time for creating transactions
    
    int numMinedblks = 0;                       // No of blks mined by the Node/peer
    ld hash_power;                              // Hash Power of the Node

    // std::ofstream log_file;
    std::vector<int> adj_peers;
    std::unordered_map<int,Link*> latency;          // Latency info 

    Block* genesis_blk;             
    Block* mining_blk;                          // Block being mined by the Node/Peer   
    chain* longest_chain;    

    std::set<Txn*,Txncomparator> txn_pool;            // Transactions waiting to be added to the block chain
    std::set<chain*,chaincomparator> tail_blks;         // All leaf blocks in the block chain, including longest chain

    std::unordered_map<int,Txn*> AllTxns;
    std::unordered_map<int,Block_Hash*> AllHashes;

    std::unordered_map<int,ld> Hashtimeout;
    std::unordered_map<int, Block*> AllBlks;  
    std::unordered_map<int, ld> recvd_time;  
    std::unordered_map<int,Block*> orphanBlks;

    std::unordered_map<int, std::vector<int>> children;
    std::unordered_map<int, std::vector<int>> orphanBlk_childs;

    std::exponential_distribution<ld> generate_Ttx, generate_tk;
    std::uniform_int_distribution<int> select_payee;
    std::uniform_real_distribution<ld> select_rand_real;

    Node() = default;
    Node(int id_, bool is_slow_ ,bool is_highhash_,ld Tt_, ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_);
    virtual ~Node();

    bool is_txn_valid(Txn* T,std::vector<int> temp_wallet);            // TBD
    void create_txn(simulator* simul,bool stop_create_events);
    void mine_new_blk(simulator* simul);
    void send_txn(int peer_id,Txn* T, simulator* simul);                // TBD
    void recv_txn(int peer_id,Txn* T, simulator* simul);       
    void recv_hash(int peer_id,Block_Hash* bhash, simulator* simul);
    void send_hash(int peer_id,Block_Hash* bhash, simulator* simul);
    virtual void recv_get_req(int peer_id,Block_Hash* bhash, simulator* simul);        
    bool is_blk_valid(Block * B,chain* c);            // TBD
    virtual Block* create_blk(simulator* simul);
    void send_blk(int peer_id, Block* B, simulator* simul);                // TBD
    virtual void recv_blk(int peer_id, Block* B, simulator* simul,bool stop_mining);
    virtual void mining_success(Block* B,simulator* simul,bool stop_mining);
    chain* create_new_chain(Block* B,simulator* simul);
    virtual void add_orphan_blks(simulator* simul);
    void printTree(int block_id,std::ostream& os, int depth = 0);
    void print_stats(simulator* simul,std::ostream &os,int ringmaster,int total_malblks);
    virtual void recv_mal_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) {};
    virtual void recv_hon_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) {};
    virtual void broadcast_private_chain(simulator* simul) {};
};

class MalNode : public Node {

public :
    Block* branch_blk;
    chain* private_chain; 
    bool eclipse_attack; 
    bool is_ringmaster;
    std::vector<int> adj_malpeers;
    std::unordered_map<int,Link*> mallatency;          // Latency info 

    MalNode(int id_, bool is_slow_ ,bool is_highhash_,bool eclipse_attack_, bool is_ringmaster_,ld Tt_, ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_);
    void recv_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) override;
    void add_orphan_blks(simulator* simul) override;
    Block* create_blk(simulator* simul) override;
    // void recv_hash(int peer_id,Block_Hash* bhash, simulator* simul);
    void recv_get_req(int peer_id,Block_Hash* bhash, simulator* simul) override;
    void mining_success(Block* B,simulator* simul,bool stop_mining) override;
    void send_private_blk(int peer_id, Block* B, simulator* simul);
    void recv_mal_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) override;
    void recv_hon_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) override;
    void broadcast_private_chain(simulator* simul) override;
    void selfish_mining(bool stop_mining,simulator* simul);
};

#endif