#include "node.h"
#include "simulator.h"

using namespace std;

Node:: Node(int id_, bool is_slow_ , ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    Ttx = Ttx_;
    Tk = Tk_;
    hash_power = hash_power_;
    genesis_blk = genesis_blk_;
    generate_Ttx = exponential_distribution<ld>(1/Ttx);
    generate_tk = exponential_distribution<ld>(1/Tk);
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

bool Node:: recv_txn(int peer_id, Txn* T, simulator* simul){

    longest_chain = *tail_blks.begin();
    if(!is_txn_valid(T,longest_chain->wallet)) return false;
    if(AllTxns.find(T->id) != AllTxns.end())  return false; 

    // cout<<"Created Events :" <<endl;
    AllTxns[T->id] = T;
    txn_pool.insert(T);
    for(int i: adj_peers){
        if(i == peer_id) {
            // cout <<"Loop-less forwarding"<<endl;
            continue;
        }
        send_txn(i,T,simul);
    }

    return true;
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
    cout << "Node :[" << id  << "] Created Events" <<endl;
    longest_chain = *tail_blks.begin();
    if(B->id == mining_blk->id && B->parent_id == longest_chain->tail->id){
        cout<<"Mining Success : ";
        cout<<*B<<endl;
        AllBlks[B->id] = B;
        for(int i:adj_peers) send_blk(i,B,simul);
        tail_blks.erase(longest_chain);
        longest_chain->update_tail(B);
        tail_blks.insert(longest_chain);
        cout << "Block added to longest chain" <<endl;
        for(Txn* T: B->Txn_list){
            txn_pool.erase(T);
        }
        if(!stop_mining) mine_new_blk(simul);
    }
    else cout<<"Mining Failure"<<endl;
}

bool Node:: is_blk_valid(Block* B, chain* c){

    if(B->get_hash() != B->hash) return false;      // if block is tampered
    vector<int> temp_wallet(c->wallet);
    for(Txn* T: B->Txn_list){
        if(c->added_txns.find(T->id) != c->added_txns.end()) return false;      // duplicate Txn
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

bool Node:: recv_blk(int peer_id, Block* B, simulator* simul,bool stop_mining){

    cout << "Node :[" << id  << "] Created Events" <<endl;
    if(AllBlks.find(B->id) != AllBlks.end()) return false;    // Block already received
    AllBlks[B->id] = B;
    longest_chain = *tail_blks.begin();
    cout << "Longest branch Tail [" << longest_chain->tail->id <<"]" <<endl;
    if(B->parent_id == longest_chain->tail->id) {
        if(!is_blk_valid(B,longest_chain)) {
            cout<<"Invalid Block" <<endl;
            return false;
        }
        for(int i:adj_peers){
            if(peer_id == i) continue;
            send_blk(i,B,simul);
        }
        tail_blks.erase(longest_chain);
        longest_chain->update_tail(B);
        tail_blks.insert(longest_chain);
        for(Txn* T: B->Txn_list){
            txn_pool.erase(T);
        }
        if(!stop_mining) mine_new_blk(simul);
        cout << "Block added to longest chain" <<endl;
        printBlockTree();
        return true;
    }

    cout << "Checking branches" <<endl;
    for(chain *c: tail_blks){
        if(c->tail->id ==  B->parent_id){
            if(!is_blk_valid(B,c)) {
                cout<<"Invalid block"<<endl;
                return false;
            }
            for(int i:adj_peers){
                if(peer_id == i) continue;
                send_blk(i,B,simul);
            }
            tail_blks.erase(c);
            c->update_tail(B);
            tail_blks.insert(c);
            if(c->depth > longest_chain->depth){
                cout <<"New Longest Branch tail - "<< c->tail->id <<endl;
                printBlockTree();
                for(int txn_id: longest_chain->added_txns){
                    txn_pool.insert(AllTxns[txn_id]);
                }
                for(int txn_id: c->added_txns){
                    txn_pool.erase(AllTxns[txn_id]);
                }
            }
            else {
                cout << "Block added to branch" <<endl;
                printBlockTree();
            }
            return true;
        }
    }

    if(AllBlks.find(B->parent_id) == AllBlks.end()){
        cout<< "Orphan Block" <<endl;
        return false;
    }

    cout <<"Creating new chain" <<endl;

    Block* temp_blk = AllBlks[B->parent_id];
    chain* c = new chain(temp_blk,0,simul->numNodes,0);
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
            cout<<"Orphan Block" <<endl;
            return false;
        }
        temp_blk = AllBlks[temp_blk->parent_id];
    }

    if(!is_blk_valid(B,c)){
        cout << "Invalid Block" <<endl;
        delete c;
        return false;
    }

    c->update_tail(B);
    tail_blks.insert(c);

    cout << "Created New Chain ending at "<< c->tail->id <<endl;
    return true;
}

ostream& operator <<(ostream& os, const Node& node) {
    os << "Node ID: " << node.id << "\n";
    // os << "Is Slow: " << (node.is_slow ? "Yes" : "No") << "\n";
    // os << "Mean Interval Time for Transactions (Ttx): " << node.Ttx << "\n";
    // os << "Number of Mined Blocks: " << node.numMinedblks << "\n";
    // os << "Hash Power: " << node.hash_power << "\n";

    // os << "Adjacent Peers: ";
    // for (const auto& peer : node.adj_peers) os << peer << " ";
    // os << "\n";

    // os << "Latency Info:\n";
    // for (const auto& [peer, link] : node.latency) 
    //     os << "  Peer " << peer << " -> Latency: " << *link << "\n";  // Assuming Link has an appropriate overload for <<

    os << "Transaction Pool:\n";
    for (const auto& txn : node.txn_pool) os << "  " << *txn << "\n";  // Assuming Txn has an appropriate overload for <<

    return os; 
}

// Recursive function to print the tree
void Node::printTree(int block_id, const std::unordered_map<int, std::vector<int>>& children, int depth) {
    std::cout << std::string(depth * 4, ' ') << "└─ Block ID: " << block_id << "\n";
    
    // Recursively print children
    if (children.find(block_id) != children.end()) {
        for (int child_id : children.at(block_id)) {
            printTree(child_id, children, depth + 1);
        }
    }
}

// Function to print the entire blockchain tree
void Node::printBlockTree() {
    std::unordered_map<int, std::vector<int>> children;
    std::vector<int> roots;

    // Find root blocks and build the child map
    for (const auto& [id, blk] : AllBlks) {
        if (AllBlks.find(blk->parent_id) == AllBlks.end()) {
            roots.push_back(id);
        }
        children[blk->parent_id].push_back(id);
    }

    // Print each root and its subtree
    for (int root_id : roots) {
        printTree(root_id, children);
    }

    cout << "Balance [ ";
    for(int i = 0; i< longest_chain->wallet.size();i++){
        cout << i << ": " << longest_chain->wallet[i] << ", " ;
    } 
    cout << " ]"<<endl;
}