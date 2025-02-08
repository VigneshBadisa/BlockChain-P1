#include "node.h"
#include "simulator.h"

using namespace std;

Node:: Node(int id_, bool is_slow_ , ld Ttx_, ld hash_power_ , simulator* simul_, Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    Ttx = Ttx_;
    hash_power = hash_power_;
    simul = simul_;
    genesis_blk = genesis_blk_;
}

void Node:: create_txn(){
}
void Node:: send_txn(){

}
void Node:: recv_txn(){

}
bool Node:: is_txn_valid(){
    return false;
}
void Node:: create_blk(){
 // creates a new block
 

}
void Node:: send_blk(){

}
void Node:: recv_blk(){

}
void Node:: update_wallet(){

}
void Node:: is_blk_valid(){
    
}
Node:: ~Node() = default;