#include "simulator.h"
#include "node.h"
#include <queue>
#include <chrono>
#include <cassert>

using namespace std;

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
int Txn::counter = 0;

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
        Node* new_node = new Node(i,slowNodes[i],T_tx,hash_power,GENESIS_blk);
        nodes[i] = new_node;
    }
    // create network topology and populate latency info  
    create_network();

    uniform_real_distribution<ld> light_delay_db(0.010,0.500);      // in seconds

    for(int i=0;i<numNodes;i++){
        for(int j:adj_nodes[i]){
            nodes[i]->adj_peers.push_back(j);
            ld pij = light_delay_db(rng),cij;
            if (!nodes[i]->is_slow && !nodes[j]->is_slow) cij = 100;
            else cij = 5;
            nodes[i]->latency[j] = new Link(j,pij,cij);
        }
        nodes[i]->select_payee = uniform_int_distribution<int>(0,numNodes-1);
    }
    // Initializing the events

    for(int i=0;i<numNodes;i++){
        Event* E = new Event(0,CREATE_TXN,i);
        // nodes[i]->mine_new_blk();
        add_event(E);
    }

    while(!event_queue.empty()){
        top_event = *event_queue.begin();
        simclock = top_event->timestamp;
        if(simclock > simEndtime) break;
        execute_event(top_event,false);            
        delete_event(top_event);
    }
    cout << "Remaining Events in the queue" <<endl;

    while(!event_queue.empty()){
        top_event = *event_queue.begin();
        execute_event(top_event,true);
        delete_event(top_event);
    }

    cout<< "End of Simulation" <<endl;
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
    // log - Network topology

    cout<<"Network printing"<<endl;
    for (int i = 0; i < numNodes; ++i) {
        cout << "Peer " << i << " connected to: ";
        for (int neighbor : adj_nodes[i]) {
            cout << neighbor << " ";
        }
        cout << endl;
    }
}
void simulator::add_event(Event * E){
    E->timestamp += simclock;
    event_queue.insert(E);
}

void simulator::delete_event(Event* E) {
    assert(E != NULL);
    auto  it = event_queue.find(E);
    assert(it != event_queue.end());
    event_queue.erase(it);
    delete E;
}

void simulator::execute_event(Event* E,bool stop_create_events){        // execute all sametime events
    if(E->type == CREATE_TXN){
        cout << *E <<endl;
        nodes[E->sender_id]->create_txn(this,stop_create_events);
    }else if (E->type == RECV_TXN){
        Event_TXN* txn_event = dynamic_cast<Event_TXN*>(E);
        bool is_executed = nodes[txn_event->receiver_id]->recv_txn(E->sender_id,txn_event->txn,this);
        if(is_executed) cout << *txn_event <<endl;
        else cout<<"Not executed"<<endl;
    }
}
void simulator::write_tree_file(){
    
}

int main(){
    simulator sim(10,10,30,5,3.0023124,324.34);
    sim.start();
    return 0;
}