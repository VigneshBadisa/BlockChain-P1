#include "simulator.h"
#include "node.h"
#include <queue>
#include <chrono>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <filesystem>

using namespace std;

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
int Txn::counter = 0;
int Block::counter = 0;

simulator:: simulator(int numNodes_, float m_,ld simEndtime_,ld Tt_, ld T_tx_, ld Tk_ ,bool eclipse_attack_){
    numNodes = numNodes_;
    m = m_;
    Tt = Tt_;
    T_tx = T_tx_;
    Tk = Tk_;
    simEndtime = simEndtime_;
    eclipse_attack = eclipse_attack_;
}

void simulator::start(){

    cout << "****Starting Simulation****"<<endl;
    // Deciding Slow Nodes
    // unordered_map<int,int> malnodes;
    int numMalNodes = (m/100)*numNodes;
    
    vector<bool> isMalNodes(numNodes,false);
    srand(time(0));
    cout << "MalNodes - ";
    for(int i=0;i<numMalNodes;i++){
        int node_id = rand() % numNodes;
        while(isMalNodes[node_id]) node_id = rand() % numNodes;
        isMalNodes[node_id] = true;
        if(i > 0) {cout << ", " << node_id;}
        else {cout << node_id;}
        malnodes[i] = node_id;
    }
    ringmaster = 0;
    while(!isMalNodes[ringmaster]) ringmaster = rand() % numNodes;
    cout << "\nRing Master - " << ringmaster << endl;
    GENESIS_blk = new Block(simclock,-1, NULL);
    ld hash_power = ld(1)/numNodes;
    // create network topology and populate latency info  
    cout << "[CHECKPOINT] : Creating Network"<<endl;
    create_network();
    cout << "[CHECKPOINT] : Netwok Created Successfully"<<endl;
    cout << "[CHECKPOINT] : Creating Mailicious Overlay Network"<<endl;
    create_malicious_network();
    cout << "[CHECKPOINT] : Overlay Netwok Created Successfully"<<endl;

    uniform_real_distribution<ld> light_delay_db(0.010,0.500);      // in seconds
    uniform_real_distribution<ld> mallight_delay_db(0.001,0.010);      // in seconds

    for(int i=0;i<numNodes;i++){
        if(!isMalNodes[i]){
            Node* new_node = new Node(i,true,false,Tt,T_tx,Tk,hash_power,GENESIS_blk);
            for(int j:adj_nodes[i]){
                new_node->adj_peers.push_back(j);
                ld pij = light_delay_db(rng),cij;
                cij = 5;
                new_node->latency[j] = new Link(j,pij,cij);
            }
            new_node->select_payee = uniform_int_distribution<int>(0,numNodes-1);
            nodes[i] = new_node;
        }else{
            MalNode*  new_node;
            if(i == ringmaster){
                new_node = new MalNode(i,false,true,eclipse_attack,true,Tt,T_tx,Tk,numMalNodes*hash_power,GENESIS_blk);
                
            }else{
                new_node = new MalNode(i,false,false,eclipse_attack,false,Tt,T_tx,Tk,0,GENESIS_blk);
            }
            for(int j:adj_nodes[i]){
                new_node->adj_peers.push_back(j);
                ld pij = light_delay_db(rng),cij;
                if(isMalNodes[j]) cij = 100;
                else cij = 5;
                new_node->latency[j] = new Link(j,pij,cij);
            }
            new_node->select_payee = uniform_int_distribution<int>(0,numNodes-1);
            for(int j:adj_malnodes[i]){
                new_node->adj_malpeers.push_back(j);
                ld pij = mallight_delay_db(rng),cij;
                cij = 100;
                new_node->mallatency[j] = new Link(j,pij,cij);
            }
            new_node->branch_blk = GENESIS_blk;
            new_node->private_chain = new chain(GENESIS_blk,0,numNodes,CAPITAL);;
            nodes[i] = new_node;
        }
        chain* c = new chain(GENESIS_blk,0,numNodes,CAPITAL);
        nodes[i]->tail_blks.insert(c);
        nodes[i]->AllBlks[0] = GENESIS_blk;
        nodes[i]->recvd_time[0] = 0;
    }


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
        if(!(isMalNodes[i] && i != ringmaster)){
            Event* E2 = new Event(START_MINING_TIME,MINING_START,i);
            add_event(E2);
        }
    }

    cout << "[CHECKPOINT] : Executing Events in queue " <<endl;

    while(simclock < simEndtime){
        if(!event_queue.empty()){
            top_event = *event_queue.begin();
            simclock = top_event->timestamp;
            // if(top_event->type == CREATE_BLK || top_event->type == RECV_BLK || top_event->type == MINING_START ) cout << *top_event <<endl;
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

    cout << "[CHECKPOINT] : Broadcasting Private Chain if present "<<endl;

    nodes[ringmaster]->broadcast_private_chain(this);

    while(!event_queue.empty()){
        // cout << *(top_event) <<endl;
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


void simulator::create_malicious_network(){
    int numMalNodes = (m/100)*numNodes;
    while (true) {
        for (int i=0;i<numMalNodes;i++) {
            adj_malnodes[malnodes[i]].clear();
        }
        // Randomly connect nodes
        for (int i = 0; i < numMalNodes; ++i) {
            while (adj_malnodes[malnodes[i]].size() < 3) {  // Ensure minimum 3 connections
                int peer = rand() % numMalNodes;
                if (peer != i && adj_malnodes[malnodes[i]].size() < 6 && adj_malnodes[malnodes[peer]].size() < 6) {
                    adj_malnodes[malnodes[i]].insert(malnodes[peer]);
                    adj_malnodes[malnodes[peer]].insert(malnodes[i]);
                }
            }
        }
        vector<bool> visited(numNodes, false);
        queue<int> q;
        q.push(malnodes[0]);
        visited[malnodes[0]] = true;
        int visitedCount = 1;

        while (!q.empty()) {
            int node = q.front();
            q.pop();
            for (int neighbor : adj_malnodes[node]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                    ++visitedCount;
                }
            }
        }
        if(visitedCount == numMalNodes) break;

    }

    // cout<<"Malicious Network printing"<<endl;
    // for (int i = 0; i < numMalNodes; ++i) {
    //     cout << "Peer " << malnodes[i] << " connected to: ";
    //     for (int neighbor : adj_malnodes[malnodes[i]]) {
    //         cout << neighbor << " ";
    //     }
    //     cout << endl;
    // }
}

void simulator::add_event(Event * E){
    E->timestamp += simclock;
    // if((E->type == RECV_BLK)) cout << *(dynamic_cast<Event_BLK*>(E)) <<endl;
    // if((E->type == RECV_MAL_BLK)) cout << *(dynamic_cast<Event_BLK*>(E)) <<endl;
    // if((E->type == GET_REQ|| E->type == RECV_HASH)) cout << *(dynamic_cast<Event_HASH*>(E)) <<endl;
    // if(E->type == BC_PRIV_CHAIN) cout <<*E <<endl;
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
    }else if( E->type == RECV_HASH){
        Event_HASH* hash_event = dynamic_cast<Event_HASH*>(E);
        // cout <<*hash_event <<endl;
        // cout << "Created - Events " <<endl;
        nodes[hash_event->receiver_id]->recv_hash(hash_event->sender_id,hash_event->bhash,this);
    }else if( E->type == GET_REQ){
        Event_HASH* hash_event = dynamic_cast<Event_HASH*>(E);
        // cout <<*hash_event <<endl;
        // cout << "Created - Events " <<endl;
        nodes[hash_event->receiver_id]->recv_get_req(hash_event->sender_id,hash_event->bhash,this);
    }else if (E->type == RECV_BLK){
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        // cout << *blk_event <<endl;
        // cout << "Created - Events " <<endl;
        if(total_mined_blks > MAX_BLOCKS){
            nodes[blk_event->receiver_id]->recv_blk(blk_event->sender_id,blk_event->blk,this,true);           
        }
        else{
            nodes[blk_event->receiver_id]->recv_blk(blk_event->sender_id,blk_event->blk,this,stop_create_events);
        } 
    }else if (E->type == RECV_MAL_BLK){
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        // cout << *blk_event <<endl;
        // cout << "Created - Events " <<endl;
        if(total_mined_blks > MAX_BLOCKS){
            nodes[blk_event->receiver_id]->recv_mal_blk(blk_event->sender_id,blk_event->blk,this,true);           
        }
        else{
            nodes[blk_event->receiver_id]->recv_mal_blk(blk_event->sender_id,blk_event->blk,this,stop_create_events);
        } 
    }else if (E->type == RECV_HON_BLK){
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        // cout << *blk_event <<endl;
        // cout << "Created - Events " <<endl;
        if(total_mined_blks > MAX_BLOCKS){
            nodes[blk_event->receiver_id]->recv_hon_blk(blk_event->sender_id,blk_event->blk,this,true);           
        }
        else{
            nodes[blk_event->receiver_id]->recv_hon_blk(blk_event->sender_id,blk_event->blk,this,stop_create_events);
        } 
    }else if (E->type == CREATE_BLK){
        Event_BLK* blk_event = dynamic_cast<Event_BLK*>(E);
        // cout << *blk_event <<endl;
        // cout << "Created - Events " <<endl;
        nodes[blk_event->receiver_id]->mining_success(blk_event->blk,this,stop_create_events);
    }else if (E->type == MINING_START){
        nodes[E->sender_id]->mine_new_blk(this);
    }else if (E->type == BC_PRIV_CHAIN){
        // cout << *E <<endl;
        // cout << "Created - Events " <<endl;
        nodes[E->sender_id]->broadcast_private_chain(this);
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
        node->print_stats(this,file,ringmaster,nodes[ringmaster]->numMinedblks);
        file.close();
    }

    ofstream file("blockchain.txt");

    if (!file) {
        cerr << "Error opening blockchain.txt" << endl;
        return;
    }

    unordered_set<int> malNodeSet;
    vector<int> honestNodes;

    // Store malicious node values in a set for quick lookup
    for (const auto& pair : malnodes) {
        malNodeSet.insert(pair.second);
    }

    // Collect all honest nodes (i.e., nodes not in malNodeSet)
    for (int i = 0; i < numNodes; i++) {
        if (malNodeSet.find(i) == malNodeSet.end()) { // If not in malNodeSet, it's honest
            honestNodes.push_back(i);
        }
    }
    random_device rd;
    mt19937 g(rd());
    shuffle(honestNodes.begin(), honestNodes.end(), g);

    int m1 = *malNodeSet.begin();
    if(m1 == ringmaster) {
        m1 = *next(malNodeSet.begin(),1);
    }

    file << honestNodes[0] << ", " << honestNodes[1] << endl;
    file << ringmaster << ", " << m1 <<endl;

    file.close();

}