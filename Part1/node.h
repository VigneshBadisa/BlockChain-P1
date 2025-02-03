#ifndef NODE_H
#define NODE_H

#include <random>

class Node {
    int id;
    bool is_slow;               // 1 - Slow , 0 - Fast
    bool is_low_cpu;            // 1 - Low CPU , 0 - High CPU


    simulator* simul;

    
    // Info maintained by each peer

    struct LATENCY_INFO{
        float pij,cij,dij_mean;
        LATENCY_INFO(float p, float c, float d){
            pij = p; cij = c; dij_mean = d;
        }
        float get_msg_latency(int msg_length){
            int seed = 123;
            default_random_engine generator (seed);
            exponential_distribution<float> distr(dij_mean);
            float dij = distr(generator);
            return pij + float(msg_length/cij) + dij;
        }
    };

    unordered_map<int,LATENCY_INFO> adj_latency;

    unordered_map<string, Txn> txn_ALL;
    vector<Txn> txn_pool;

    int n_blks_generated = 0;
    float avg_time_btw_txns;

    Block genesis_blk;
    unordered_map<string, Block> blk_ALL;
    unordered_map<string,pair<int,Block>> blks_unvalidated;

    unordered_map<string,unordered_map<int,int>> balance_ALL;

    Node(int node_id, bool speed, bool cpu, simulator* sim){
        id = node_id;
        speed = is_slow;
        cpu = is_low_cpu;
        simul = sim;
        avg_time_btw_txns = simul->T_tx;
        genesis_blk = simul->GENESIS_blk;
        blk_ALL[genesis_blk.curr_block_hash] = genesis_blk;
    }
    ~Node();

    void add_new_latency(int node_id, float pij, float cij, float dij_mean);
    void update_balance();
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