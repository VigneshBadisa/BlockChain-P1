#include "simulator.h"
#include "node.h"
#include <queue>
#include <chrono>
#include <cassert>
#include <fstream>
#include <filesystem>

using namespace std;

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
int Txn::counter = 0;
int Block::counter = 0;

void simulator::start(){

    cout << "****Starting Simulation****"<<endl;
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

    GENESIS_blk = new Block(simclock,-1, NULL);

    for(int i=0;i<numNodes;i++){
        ld hash_power = low_hash_power;
        if(!lowhashNodes[i]) hash_power = 10*low_hash_power;
        Node* new_node = new Node(i,slowNodes[i],!lowhashNodes[i],T_tx,Tk,hash_power,GENESIS_blk);
        nodes[i] = new_node;
        chain* c = new chain(GENESIS_blk,0,numNodes,CAPITAL);
        nodes[i]->tail_blks.insert(c);
        nodes[i]->AllBlks[0] = GENESIS_blk;
        nodes[i]->recvd_time[0] = 0;
    }
    // create network topology and populate latency info  
    cout << "[CHECKPOINT] : Creating Network"<<endl;
    create_network();
    cout << "[CHECKPOINT] : Netwok Created Successfully"<<endl;

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

    cout << "[CHECKPOINT] : Initializing Events - Mining Starts after 5 sec" <<endl;

    for(int i=0;i<numNodes;i++){
        Event* E = new Event(0,CREATE_TXN,i);
        add_event(E);
        Event* E2 = new Event(START_MINING_TIME,MINING_START,i);
        add_event(E2);
    }

    cout << "[CHECKPOINT] : Executing Events in queue " <<endl;

    while(simclock < simEndtime){
        if(!event_queue.empty()){
            top_event = *event_queue.begin();
            simclock = top_event->timestamp;
            execute_event(top_event,false);            
            delete_event(top_event);
        }      
    }

    cout << "[CHECKPOINT] : All Events before simEndtime executed Succesfully" <<endl;

    cout << "[CHECKPOINT] : Stopping Creating Events"<<endl;

    while(!event_queue.empty()){
        top_event = *event_queue.begin();
        simclock = top_event->timestamp;
        execute_event(top_event,true);
        delete_event(top_event);
    }
    
    cout << "[CHECKPOINT] : Remaining Events in queue executed Succesfully" <<endl;

    cout<< "[CHECKPOINT] : End of Simulation" <<endl;

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

    // cout<<"Network printing"<<endl;
    // for (int i = 0; i < numNodes; ++i) {
    //     cout << "Peer " << i << " connected to: ";
    //     for (int neighbor : adj_nodes[i]) {
    //         cout << neighbor << " ";
    //     }
    //     cout << endl;
    // }
}
void simulator::add_event(Event * E){
    E->timestamp += simclock;
    // if(E->type == CREATE_BLK || E->type == RECV_BLK )cout<< *E <<endl;
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
        // cout << *E <<endl;
        if(total_transactions > MAX_TRANSACTIONS){
            nodes[E->sender_id]->create_txn(this,true);
        }
        else{
            nodes[E->sender_id]->create_txn(this,stop_create_events);
            if(!stop_create_events)  total_transactions += 1;
        } 
    }else if (E->type == RECV_TXN){
        Event_TXN* txn_event = dynamic_cast<Event_TXN*>(E);
        nodes[txn_event->receiver_id]->recv_txn(txn_event->sender_id,txn_event->txn,this);
        // if(is_executed) cout << *txn_event <<endl;
        // else cout<<"Txn discarded"<<endl;
    }else if (E->type == RECV_BLK){
        // cout << "New Event " <<endl;
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        if(total_mined_blks > MAX_BLOCKS){
            nodes[blk_event->receiver_id]->recv_blk(blk_event->sender_id,blk_event->blk,this,true);           
        }
        else{
            nodes[blk_event->receiver_id]->recv_blk(blk_event->sender_id,blk_event->blk,this,stop_create_events);
        } 

        // cout << *blk_event <<endl;
        // if(!is_executed) cout<<" Block discarded "<<endl;
        // cout << "Block chain of "<<blk_event->receiver_id << endl;
        // nodes[blk_event->receiver_id]->printBlockTree();
    }else if (E->type == CREATE_BLK){
        // cout << "New Event " <<endl;
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        // cout << *blk_event <<endl;
        nodes[blk_event->receiver_id]->mining_success(blk_event->blk,this,stop_create_events);
        // cout << "Block chain of "<<blk_event->receiver_id << endl;
        // nodes[blk_event->receiver_id]->printBlockTree();
    }else if (E->type == MINING_START){
        // cout << "New Event " <<endl;
        // cout<<*E<<endl;
        // cout<< "Node : [" << E->sender_id << "] Created Events "<<endl;
        nodes[E->sender_id]->mine_new_blk(this);
    }

}
void simulator::write_tree_file(){

    string logDir = "logs";  // Directory name

    // Check if "logs" directory exists; if not, create it
    if (!filesystem::exists(logDir)) {
        if (filesystem::create_directory(logDir)) {
            cout << "Directory 'logs' created successfully.\n";
        } else {
            cerr << "Failed to create 'logs' directory.\n";
            return;
        }
    }

    for (const auto& [id, node] : nodes) {
        ofstream file("logs/Peer" + to_string(id) + ".log");
        if (!file) {
            cerr << "Error opening Peer.txt" << endl;
            return;
        }
        node->print_stats(this,file);
        file.close();
    }

    ofstream file("blockchain.txt");

    if (!file) {
        cerr << "Error opening blockchain.txt" << endl;
        return;
    }
    for (const auto& [parent, child_list] : nodes[0]->children) {
        for (int child : child_list) {
            file << parent << " " << child << "\n";
        }
    }

    file.close();

}