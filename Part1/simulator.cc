#include "simulator.h"
#include "node.h"
#include <queue>

using namespace std;

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

    GENESIS_blk = new Block(0, simclock, "", NULL);

    for(int i=0;i<numNodes;i++){
        ld hash_power = low_hash_power;
        if(!lowhashNodes[i]) hash_power = 10*low_hash_power;
        Node* new_node = new Node(i,slowNodes[i],T_tx,hash_power,this,GENESIS_blk);
        nodes[i] = new_node;
    }
    // create network topology and populate latency info  
    create_network();

    uniform_real_distribution<ld> light_delay_db(0.010,0.500);

    default_random_engine generator;

    for(int i=0;i<numNodes;i++){
        for(int j:adj_nodes[i]){
            ld pij = light_delay_db(generator),cij;
            if (!nodes[i]->is_slow && !nodes[j]->is_slow) cij = 100;
            else cij = 5;
            nodes[i]->latency[j] = make_pair(pij,cij);
        }
    }
    // 

}

// Create a connected graph as per instrcutions in Point 4 and 
void simulator::create_network(){
    while (true) {

        for (int i=0;i<numNodes;i++) {
            adj_nodes[i].clear();
        }
        // Randomly connect nodes
        for (int i = 0; i < numNodes; ++i) {
            while (adj_nodes[i].size() < 3) {  // Ensure minimum 3 connections
                int peer = rand() % numNodes;
                if (peer != i && adj_nodes[i].size() < 6 && adj_nodes[peer].size() < 6) {
                    adj_nodes[i].insert(peer);
                    adj_nodes[peer].insert(i);
                }
            }
        }
        vector<bool> visited(numNodes, false);
        queue<int> q;
        q.push(0);
        visited[0] = true;
        int visitedCount = 1;

        while (!q.empty()) {
            int node = q.front();
            q.pop();
            for (int neighbor : adj_nodes[node]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                    ++visitedCount;
                }
            }
        }
        if(visitedCount == numNodes) break;
    }
    cout<<"Network printing"<<endl;
    for (int i = 0; i < numNodes; ++i) {
        cout << "Peer " << i << " connected to: ";
        for (int neighbor : adj_nodes[i]) {
            cout << neighbor << " ";
        }
        cout << endl;
    }
}
void simulator::add_event(){

}
void simulator::execute_top_event(){

}
void simulator::write_tree_file(){
    
}

int main(){
    simulator sim(10,10,30,3.0023124,324.34);
    sim.start();
    return 0;
}