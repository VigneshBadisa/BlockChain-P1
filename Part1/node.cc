#include "node.h"

using namespace std;

void Node:: add_new_latency(int node_id, float pij, float cij, float dij_mean){
    adj_latency[node_id] = LATENCY_INFO(pij,cij,dij_mean);
}
void Node:: create_txn(){

}
void Node:: send_txn(){

}
void Node:: recv_txn(){

}
bool Node:: is_txn_valid(){

}
void Node:: create_blk(){

}
void Node:: send_blk(){

}
void Node:: recv_blk(){

}
void Node:: update_balance(){

}
void Node:: is_blk_valid(){
    
}
Node:: ~Node(){

}