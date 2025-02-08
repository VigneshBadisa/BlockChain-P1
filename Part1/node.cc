#include "node.h"
#include "simulator.h"

using namespace std;

Node:: Node(int id_, bool is_slow_ , ld Ttx_, ld hash_power_ , Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    Ttx = Ttx_;
    hash_power = hash_power_;
    genesis_blk = genesis_blk_;
    generate_Ttx = exponential_distribution<ld>(1/Ttx);
    select_rand_real = uniform_real_distribution<ld>(0,1);
}


// Validating a transaction

bool Node:: is_txn_valid(Txn* T){
    if(wallet[T->id] >= T->amount) return true;
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

    if(!is_txn_valid(T)) return false;
    if(Alltxns.find(T->id) != Alltxns.end())  return false; 

    // cout<<"Created Events :" <<endl;
    Alltxns.insert(T->id);
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

    // cout<<"Created Events:" <<endl;
    int amount_ = select_rand_real(rng)*wallet[id];
    int payee_id_ = select_payee(rng);
    while(payee_id_ == id){
        payee_id_ = select_payee(rng);
    }
    Txn* T = new Txn(simul->simclock,id,payee_id_,amount_);
    // Adding Txn to my pool
    Alltxns.insert(T->id);
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


void Node:: create_blk(){

}
void Node:: send_blk(){

}
void Node:: recv_blk(){

}
void Node:: update_wallet(){

}
void Node:: is_blk_valid(Block* B){
    
}

ostream& operator <<(ostream& os, const Node& node) {
    os << "Node ID: " << node.id << "\n";
    os << "Is Slow: " << (node.is_slow ? "Yes" : "No") << "\n";
    os << "Mean Interval Time for Transactions (Ttx): " << node.Ttx << "\n";
    os << "Number of Mined Blocks: " << node.numMinedblks << "\n";
    os << "Hash Power: " << node.hash_power << "\n";

    os << "Adjacent Peers: ";
    for (const auto& peer : node.adj_peers) os << peer << " ";
    os << "\n";

    os << "Latency Info:\n";
    for (const auto& [peer, link] : node.latency) 
        os << "  Peer " << peer << " -> Latency: " << *link << "\n";  // Assuming Link has an appropriate overload for <<

    // os << "Genesis Block ID: " << (node.genesis_blk ? node.genesis_blk->id : -1) << "\n";
    // os << "Tail Block ID: " << (node.tail_blk ? node.tail_blk->id : -1) << "\n";
    // os << "Currently Mining Block ID: " << (node.mining_blk ? node.mining_blk->id : -1) << "\n";

    // os << "All Transactions: ";
    // for (const auto& txn_id : node.Alltxns) os << txn_id << " ";
    // os << "\n";

    // os << "Transaction Pool:\n";
    // for (const auto& txn : node.txn_pool) os << "  " << *txn << "\n";  // Assuming Txn has an appropriate overload for <<

    // os << "Blockchain Blocks: ";
    // for (const auto& blk_id : node.blockchain) os << blk_id << " ";
    // os << "\n";

    // os << "Orphan Blocks: ";
    // for (const auto& blk_id : node.orphan_blks) os << blk_id << " ";
    // os << "\n";

    // os << "All Blocks:\n";
    // for (const auto& [blk_id, block] : node.AllBlks) os << "  Block ID " << blk_id << " -> " << *block << "\n";  // Assuming Block has an appropriate overload for <<

    // os << "Wallet Balances:\n";
    // for (const auto& [peer, balance] : node.wallet) os << "  Peer " << peer << " -> Balance: " << balance << "\n";

    return os; 
}