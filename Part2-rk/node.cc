#include "node.h"
#include "simulator.h"
#include <queue>

using namespace std;

Node:: Node(int id_, bool is_slow_ ,bool is_highhash_,ld Tt_, ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    is_highhash = is_highhash_;
    Tt = Tt_,
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

//  Sending get_req only once per hash received -- TBM

void Node::recv_hash(int peer_id,Block_Hash* bhash, simulator* simul){
    if(AllHashes.find(bhash->id) != AllHashes.end()){
        if(AllBlks.find(bhash->id) != AllBlks.end()) return;
        int msg_len = bhash->size();
        ld latency_ij = latency[peer_id]->get_delay(msg_len);
        if(simul->simclock - Hashtimeout[bhash->id] > Tt){            
            Event_HASH* E = new Event_HASH(latency_ij,GET_REQ,id,peer_id,bhash);
            // cout <<"[" << simul->simclock << "] Node :[" << id  << "] Hash : [" << bhash->id << "] - Timeout expired :"  <<endl;
            Hashtimeout[bhash->id] = simul->simclock;
            simul->add_event(E);
        }else{
            ld delay = Hashtimeout[bhash->id] + Tt - simul->simclock + latency_ij;
            // cout <<"[" << simul->simclock << "] Node :[" << id  << "] Hash : [" << bhash->id << "] - Timeout still running" <<endl;
            Event_HASH* E = new Event_HASH(delay,GET_REQ,id,peer_id,bhash);
            Hashtimeout[bhash->id] = simul->simclock;
            simul->add_event(E);    
        }
    }else{
        if(AllBlks.find(bhash->id) != AllBlks.end()){
            AllHashes[bhash->id] = bhash;
            return;
        }
        // cout <<"[" << simul->simclock << "] Node :[" << id  << "] Hash : [" << bhash->id << "] - Initializing timeout :" <<endl;
        AllHashes[bhash->id] = bhash;
        int msg_len = bhash->size();
        ld latency_ij = latency[peer_id]->get_delay(msg_len);
        Event_HASH* E = new Event_HASH(latency_ij,GET_REQ,id,peer_id,bhash);
        Hashtimeout[bhash->id] = simul->simclock;
        simul->add_event(E);
    }
}

void Node::send_hash(int peer_id,Block_Hash* bhash, simulator* simul){
    int msg_len = bhash->size();
    ld latency_ij = latency[peer_id]->get_delay(msg_len);
    Event_HASH* E = new Event_HASH(latency_ij,RECV_HASH,id,peer_id,bhash);
    simul->add_event(E);
}

void Node::recv_get_req(int peer_id,Block_Hash* bhash, simulator* simul){
    Block* B = AllBlks[bhash->id];
    send_blk(peer_id,B,simul);
}

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
    ld delay = generate_tk(rng);
    // cout << "Node [" << id << "] : GEN BLK - delay - "<<delay <<endl;
    Event_BLK* E = new Event_BLK(delay,CREATE_BLK,id,id,mining_blk);
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
        // for(int i:adj_peers) send_blk(i,B,simul);
        Block_Hash* bhash = new Block_Hash(B->id,B->hash);
        AllHashes[B->id] = bhash;
        for(int i:adj_peers) send_hash(i,bhash,simul);
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
                    // cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan Blk :["<< i << "], Parent ID :[" << c->tail->id << "] Added to chain " <<endl;
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
    if(AllHashes.find(B->id) == AllHashes.end()){
        Block_Hash* bhash = new Block_Hash(B->id,B->hash);
        AllHashes[B->id] = bhash;
    }
    for(int i:adj_peers){
        if(peer_id == i) continue;
        send_hash(i,AllHashes[B->id],simul);
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
        if(!stop_mining) {
            mine_new_blk(simul);
        }

        return;
    }

    // cout << "Checking branches" <<endl;
    for(chain *c: tail_blks){
        if(c == NULL || c == nullptr) {cout <<"c is NULL"<<endl;}
        if(c->tail == NULL || c->tail == nullptr) {cout <<"tail blk is NULL"<<endl;}
        // cout << c->tail->id <<endl;
        if(c->tail->id ==  B->parent_id){
            // cout << "Check 1" <<endl;
            if(!is_blk_valid(B,c)) {
                cout<< "[" << simul->simclock << "] Node :[" << id  << "] 2:Invalid" << *B <<endl;
                return;
            }
            // cout << "Check 2" <<endl;
            c->update_tail(B);
            longest_chain = *tail_blks.begin();
            // cout << "Check 3" <<endl;
            if(c->depth != longest_chain->depth) {
                // cout << "Check 4" <<endl;
                tail_blks.erase(c);
                tail_blks.insert(c);
            }
            // cout << "Check 5" <<endl;
            children[B->parent_id].push_back(B->id);
            add_orphan_blks(simul);
            // cout << "Check 6" <<endl;
            return;
        }
    }


    if(AllBlks.find(B->parent_id) == AllBlks.end()){
        // cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan Blk :["<< B->id << "], Parent ID:[" << B->parent_id << "] found " <<endl;
        orphanBlks[B->id] = B;
        orphanBlk_childs[B->parent_id].push_back(B->id);
        return;
    }

    // cout <<"Creating new chain" <<endl;

    chain* c = create_new_chain(AllBlks[B->parent_id],simul);

    if (c == nullptr){
        // cout << "[" << simul->simclock << "] Node :[" << id  << "] Orphan chain found " <<endl;
        orphanBlks[B->id] = B;
        orphanBlk_childs[B->parent_id].push_back(B->id);
        return;
    }

    tail_blks.insert(c);
    c->update_tail(B);
    if(c->depth != longest_chain->depth) {
        tail_blks.erase(c);
        tail_blks.insert(c);
    }
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

void Node::print_stats(simulator* simul,ostream &os, int ringmaster,int total_malblks){

    os << "Node ID: " << id << "\n";
    os << "Number of Mined Blocks: " << numMinedblks << "\n";
    os << "Hash Power: " << hash_power << "\n";

    
    longest_chain = *tail_blks.begin();
    os << "Longest chain length : " << longest_chain->depth <<endl;

    int blk_id = longest_chain->tail->id;
    int numMalBlks = 0;
    while(blk_id != 0){
        int miner = AllBlks[blk_id]->Txn_list[0]->payee_id;
        if(miner == ringmaster) numMalBlks++;
        blk_id = AllBlks[blk_id]->parent_id;
    }
    os << "Ratio of No of Malicious blks to Total Blks in Longest chain - "<< float(numMalBlks)/longest_chain->depth <<endl;
    os << "Ratio of No of Malicious blks in Longest chain to total Mal blks - "<< float(numMalBlks)/total_malblks <<endl;
    printTree(0,os,0);
}
