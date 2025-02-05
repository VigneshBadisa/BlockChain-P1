#include "simulator.h"
#include "node.h"

using namespace std;

class Node;

void simulator::start(){
    // Deciding Slow Nodes
    int numSlowNodes = (z0/100.0)*numNodes;
    vector<bool> slowNodes(numNodes,false);
    vector<bool> lowhashNodes(numNodes,false);
    srand(time(0));
    for(int i=0;i<numSlowNodes;i++){
        int node_id = rand() % numNodes;
        slowNodes[node_id] = true;
    }
    // Computing hash power of each node
    int numlowNodes = (z1/100.0)* numNodes;
    ld low_hash_power = 1.0/(10*numNodes - 9*numlowNodes);
    for(int i=0;i<numlowNodes;i++){
        int node_id = rand() % numNodes;
        lowhashNodes[node_id] = true;
    }
    create_genesis_blk();
    for(int i=0;i<numNodes;i++){
        ld hash_power = low_hash_power;
        if(!lowhashNodes[i]) hash_power = 10*low_hash_power;
        Node* new_node = new Node(i,slowNodes[i],T_tx,hash_power,this,GENESIS_blk);
        nodes[i] = new_node;
    }
    create_network();
}

// Create a connected graph as per instrcutions in Point 4 and 
void simulator::create_network(){

}
void simulator::create_genesis_blk(){

}
void simulator::add_event(){

}
void simulator::execute_top_event(){

}
void simulator::write_tree_file(){
    
}
simulator:: ~simulator(){

}

int main(){
    simulator sim(10,10,30,3.0023124,324.34);
    sim.start();
    return 0;
}