#include "node.h"
#include "simulator.h"
#include <queue>

using namespace std;

Node:: Node(int id_, bool is_slow_ ,bool is_highhash_, ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    is_highhash = is_highhash_;
    Ttx = Ttx_;
    Tk = Tk_;
    hash_power = hash_power_;
    genesis_blk = genesis_blk_;
    generate_Ttx = exponential_distribution<ld>(1/Ttx);
    generate_tk = exponential_distribution<ld>(hash_power/Tk);
    select_rand_real = uniform_real_distribution<ld>(0,1);
}


// Validating a transaction

bool Node:: is_txn_valid(Txn* T, vector<int> temp_wallet){
    if(T->payee_id == T->payer_id){
       if(T->amount == MINING_FEE)  return true;
       else {
        cout << "Wrong Mining Fee" <<endl;
        return false;
       }
    }  // Coinbase Transaction
    if(temp_wallet[T->payer_id] >= T->amount) return true;
    return false;
}

// Send Transaction to a peer

void Node:: send_txn(int peer_id, Txn* T,simulator* simul){
    int msg_len = T->size();
    ld latency_ij = latency[peer_id]->get_delay(msg_len);
    Event_TXN* E = new Event_TXN(latency_ij,RECV_TXN,id,peer_id,T);
    // log - adding new event - SEND and RECV Txn
    // cout<<*E<<endl;
    simul->add_event(E);
}

// Receive Txn from peer and loop-less forward it

void Node:: recv_txn(int peer_id, Txn* T, simulator* simul){

    // longest_chain = *tail_blks.begin();
    // if(!is_txn_valid(T,longest_chain->wallet)) return ;
    if(AllTxns.find(T->id) != AllTxns.end())  return ; 
    // cout<<"Created Events :" <<endl;
    AllTxns[T->id] = T;
    txn_pool.insert(T);
    // cout << "Node - [" <<id <<"] Txn added - [" << T->id << "]" <<endl;
    for(int i: adj_peers){
        if(i == peer_id) {
            // cout <<"Loop-less forwarding"<<endl;
            continue;
        }
        send_txn(i,T,simul);
    }

    return ;
}

// Creates a transaction adds RECV_TXN to event queue with latency delay 
// Also adds a CREATE_TXN with delay from exp(Ttx) as mentioned

void:: Node::create_txn(simulator* simul,bool stop_create_events){

    // Assuming wallet has money
    // Add code to handle how initial Txn's are handled

    longest_chain = *tail_blks.begin();
    int amount_ = select_rand_real(rng)*longest_chain->wallet[id];
    int payee_id_ = select_payee(rng);
    while(payee_id_ == id){     // don't pay to yourself
        payee_id_ = select_payee(rng);
    }
    Txn* T = new Txn(simul->simclock,id,payee_id_,amount_);
    // Adding Txn to my pool
    AllTxns[T->id] = T;
    // cout << "Node - [" <<id <<"] Txn added - [" << T->id << "]" <<endl;
    txn_pool.insert(T);
    for(int i : adj_peers){
        send_txn(i,T,simul);
    }

    if(!stop_create_events){
        Event * E = new Event(generate_Ttx(rng),CREATE_TXN,id);
        // log - adding new Event
        // cout << *E <<endl;
        simul->add_event(E);
    }

}

//  Adding the coin base Txn along with Txn's from txn_pool

Block* Node:: create_blk(simulator* simul){

    longest_chain = *tail_blks.begin();
    int parent_id = longest_chain->tail->id;
    vector<Txn*> txn_list;
    Txn* T = new Txn(simul->simclock,id,id,50); // Coinbase Txn
    txn_list.push_back(T);
    int size = 1;
    vector<int> temp_wallet(longest_chain->wallet);
    for(Txn* T: txn_pool){
        if (size >= MAX_BLK_SIZE) break;
        if(is_txn_valid(T,temp_wallet)) {
            txn_list.push_back(T);
            temp_wallet[T->payer_id] -= T->amount;
            temp_wallet[T->payee_id] += T->amount;
            size += 1;
        }
    }
    Block* B = new Block(simul->simclock,parent_id,&txn_list);
    // cout << "Created Block : " << *B << endl;
    return B;

}

void Node:: send_blk(int peer_id, Block* B, simulator* simul){
    int msg_len = B->size();
    ld latency_ij = latency[peer_id]->get_delay(msg_len);
    Event_BLK* E = new Event_BLK(latency_ij,RECV_BLK,id,peer_id,B);
    simul->add_event(E);
}

void Node::mine_new_blk(simulator* simul){
    mining_blk = create_blk(simul);
    Event_BLK* E = new Event_BLK(generate_tk(rng),CREATE_BLK,id,id,mining_blk);
    simul->add_event(E);
}

void Node::mining_success(Block* B,simulator* simul,bool stop_mining){
    longest_chain = *tail_blks.begin();
    recvd_time[B->id] = simul->simclock;
    if(B->id == mining_blk->id && B->parent_id == longest_chain->tail->id){
        numMinedblks += 1;
        simul->total_mined_blks += 1;
        cout <<"[" << simul->simclock << "] Node :[" << id  << "] - ";
        cout<<"Mining Success : ";
        cout<<*B<<endl;
        AllBlks[B->id] = B;
        for(int i:adj_peers) send_blk(i,B,simul);
        tail_blks.erase(longest_chain);
        longest_chain->update_tail(B);
        tail_blks.insert(longest_chain);
        // cout << "Block added to longest chain" <<endl;
        children[B->parent_id].push_back(B->id);
        for(Txn* T: B->Txn_list){
            txn_pool.erase(T);
        }
        if(!stop_mining) mine_new_blk(simul);
    }
    // else cout<<"Mining Failure"<<endl;
}

bool Node:: is_blk_valid(Block* B, chain* c){

    if(B->get_hash() != B->hash) return false;      // if block is tampered
    vector<int> temp_wallet(c->wallet);
    for(Txn* T: B->Txn_list){
        if(c->added_txns.find(T->id) != c->added_txns.end()) {
            cout << "Duplicate Transaction Found" <<endl;
            return false;      // duplicate Txn
        }
        if(is_txn_valid(T,temp_wallet)){
            temp_wallet[T->payer_id] -= T->amount;
            temp_wallet[T->payee_id] += T->amount;
        }else{
            cout << "Invalid Transaction found : " <<*T<<endl;
            return false;
        }
    }
    return true;
}

chain* Node:: create_new_chain(Block* B,simulator* simul){

    Block* temp_blk = B;
    chain* c = new chain(temp_blk,0,simul->numNodes,CAPITAL);
    while(temp_blk->id != 0){
        c->depth += 1;
        for(Txn* T: temp_blk->Txn_list){
            if(T->payer_id == T->payee_id){
                c->wallet[T->payer_id] += T->amount;
            }
            else{
                c->added_txns.insert(T->id);
                c->wallet[T->payer_id] -= T->amount;
                c->wallet[T->payee_id] += T->amount;
            }
        }
        if(AllBlks.find(temp_blk->parent_id) == AllBlks.end()) {
            delete c;
            // cout<<" Orphan Block 2" <<endl;
            return nullptr;
        }
        temp_blk = AllBlks[temp_blk->parent_id];
    }

    return c;

}

void Node:: add_orphan_blks(simulator* simul){

    bool all_chains_made = false;

    while(!all_chains_made){
        all_chains_made = true;
        for(chain* c: tail_blks){
            if(orphanBlk_childs.find(c->tail->id) != orphanBlk_childs.end()){

                tail_blks.erase(c);
                for(int i: orphanBlk_childs[c->tail->id]){
                    cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan Blk :["<< i << "], Parent ID :[" << c->tail->id << "] Added to chain " <<endl;
                    if(!is_blk_valid(AllBlks[i],c)) {
                        cout<<"[" << simul->simclock << "] Node :[" << id  << "] 3:Invalid" << *AllBlks[i] <<endl;
                        return;
                    }
                    chain * new_chain = new chain(AllBlks[i],c);
                    children[c->tail->id].push_back(i);
                    tail_blks.insert(new_chain);
                }
                all_chains_made = false;
                orphanBlk_childs[c->tail->id].clear();
                orphanBlk_childs.erase(c->tail->id);
                delete c;
                break;
            }
        }
    }

    chain* new_longest_chain = *tail_blks.begin();
        // cout <<"New Longest Branch tail - "<< new_longest_chain->tail->id <<endl;
    // printBlockTree();
    for (int txn_id : longest_chain->added_txns) {
        auto it = AllTxns.find(txn_id);
        if (it != AllTxns.end()) {  // Only insert if txn_id exists
            txn_pool.insert(it->second);
        }else{
            // cout << "Node :[" << id  << "] Transaction not found 1:[" << txn_id <<"]" <<endl;
        }
    }
    
    for (int txn_id : new_longest_chain->added_txns) {
        auto it = AllTxns.find(txn_id);
        if (it != AllTxns.end()) {  // Only erase if txn_id exists
            txn_pool.erase(it->second);
        } else{
            // cout << "Node :[" << id  << "] Transaction not found 2:[" << txn_id <<"]" <<endl;
        }
    }

}

void Node:: recv_blk(int peer_id, Block* B, simulator* simul,bool stop_mining){

    // cout << "Node :[" << id  << "] Created Events" <<endl;
    if(AllBlks.find(B->id) != AllBlks.end()) return;    // Block already received
    AllBlks[B->id] = B;
    for(int i:adj_peers){
        if(peer_id == i) continue;
        send_blk(i,B,simul);
    }
    recvd_time[B->id]= simul->simclock;
    longest_chain = *tail_blks.begin();
    // cout << "Longest branch Tail [" << longest_chain->tail->id <<"]" <<endl;
    if(B->parent_id == longest_chain->tail->id) {
        if(!is_blk_valid(B,longest_chain)) {
            cout<<"[" << simul->simclock << "] Node :[" << id  << "] 1:Invalid" << *B <<endl;
            return;
        }
        tail_blks.erase(longest_chain);
        longest_chain->update_tail(B);
        tail_blks.insert(longest_chain);
        // cout << "Block added to longest chain" <<endl;
        // printBlockTree();
        children[B->parent_id].push_back(B->id);
        add_orphan_blks(simul);
        if(!stop_mining) mine_new_blk(simul);

        return;
    }

    // cout << "Checking branches" <<endl;
    for(chain *c: tail_blks){
        if(c->tail->id ==  B->parent_id){
            if(!is_blk_valid(B,c)) {
                cout<< "[" << simul->simclock << "] Node :[" << id  << "] 2:Invalid" << *B <<endl;
                return;
            }
            tail_blks.erase(c);
            c->update_tail(B);
            tail_blks.insert(c);
            children[B->parent_id].push_back(B->id);
            add_orphan_blks(simul);
            return;
        }
    }


    if(AllBlks.find(B->parent_id) == AllBlks.end()){
        cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan Blk :["<< B->id << "], Parent ID:[" << B->parent_id << "] found " <<endl;
        orphanBlks[B->id] = B;
        orphanBlk_childs[B->parent_id].push_back(B->id);
        return;
    }

    // cout <<"Creating new chain" <<endl;

    chain* c = create_new_chain(B,simul);

    if (c == nullptr){
        cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan chain found " <<endl;
        orphanBlks[B->id] = B;
        orphanBlk_childs[B->parent_id].push_back(B->id);
        return;
    }

    tail_blks.insert(c);
    children[B->parent_id].push_back(B->id);
    add_orphan_blks(simul);
    return ;
}


// Recursive function to print the tree
void Node::printTree(int block_id, ostream &os, int depth) {
    int miner;
    if(block_id == 0){
        miner = -1;
    }else{
        miner = AllBlks[block_id]->Txn_list[0]->payee_id;
    }
    os << "Block ID: " << block_id << ", Parent ID: " << AllBlks[block_id]->parent_id << ", Mined by: " << miner << ", RECVD-TIME: " << recvd_time[block_id] << "\n";
    
    // Recursively print children
    if (children.find(block_id) != children.end()) {
        for (int child_id : children.at(block_id)) {
            printTree(child_id,os, depth + 1);
        }
    }
}

Node::~Node(){
    // Free memory allocated for latency links
    for (auto& pair : latency) {
        delete pair.second;
    }
    latency.clear();

    // Free all transactions
    for (auto& pair : AllTxns) {
        delete pair.second;
    }
    AllTxns.clear();

    // Free all blocks
    for (auto& pair : AllBlks) {
        delete pair.second;
    }
    AllBlks.clear();

    // Free chains
    for (auto chain_ptr : tail_blks) {
        delete chain_ptr;
    }
    tail_blks.clear();

    // Free the block being mined
    delete mining_blk;
}

void Node::print_stats(simulator* simul,ostream &os){

    int lowHash_blks = 0;
    int highHash_blks = 0;
    int slowNode_blks = 0;
    int fastNode_blks = 0;

    os << "Node ID: " << id << "\n";
    os << "Is Slow: " << (is_slow ? "Yes" : "No") << "\n";
    os << "Is High hash power: " << (is_highhash ? "Yes" : "No") << "\n";
    os << "Mean Interval Time for Transactions (Ttx): " << Ttx << "\n";
    os << "Average Time for Mining (Tk): " << Tk << "\n";
    os << "Number of Mined Blocks: " << numMinedblks << "\n";
    os << "Hash Power: " << hash_power << "\n";

    vector<int> cnt_blks = vector<int>(simul->numNodes,0);
    
    longest_chain = *tail_blks.begin();
    int blk_id = longest_chain->tail->id;

    while(blk_id != 0){
        cnt_blks[AllBlks[blk_id]->Txn_list[0]->payee_id]++;
        blk_id = AllBlks[blk_id]->parent_id;
    }
    os << "Total No of mined blocks : " << simul->total_mined_blks <<endl;
    os << "Longest chain length : " << longest_chain->depth <<endl;

    os << "No of mined blocks of each node in longest chain : ";
    for(int i =0 ; i < simul->numNodes ; i++){
        if(simul->nodes[i]->is_slow) slowNode_blks += cnt_blks[i];
        else fastNode_blks += cnt_blks[i];
        if(simul->nodes[i]->is_highhash) highHash_blks += cnt_blks[i];
        else lowHash_blks += cnt_blks[i];
        os << cnt_blks[i] << " ";
    }
    os <<endl;
    os << "Total No of Blocks mined by Slow Nodes :" << slowNode_blks <<endl;
    os << "Total No of Blocks mined by Fast Nodes :" << fastNode_blks <<endl;
    os << "Total No of Blocks mined by High hash power Nodes :" << highHash_blks <<endl;
    os << "Total No of Blocks mined by Low Hash power Nodes :" << lowHash_blks <<endl;

    printTree(0,os,0);
}