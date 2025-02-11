#include "helper.h"

#include <sstream>
#include <openssl/sha.h>
#include <random>

using namespace std;


Txn::Txn(ld timestamp_ ,int payer_id_, int payee_id_, int amount_){
    id = counter++;
    payer_id = payer_id_;
    payee_id = payee_id_;
    amount = amount_;
    timestamp = timestamp_;
}

string Txn:: get_string(){
    ostringstream ss;
    ss << timestamp << payer_id << payee_id << amount;
    return ss.str();    
}

string Txn:: get_hash(){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    string txn_string = get_string();
    SHA256(reinterpret_cast<const unsigned char *>(txn_string.c_str()),txn_string.size(),hash);
    // Convert to human readable string
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int Txn::size(){ 
// Size of the transaction assumed to be 1KB
    return 1000;
}

Block::Block(ld timestamp_ , int parent_id_,vector<Txn*>*Txn_list_){
    id = counter++;
    timestamp = timestamp_;
    parent_id = parent_id_;
    if(Txn_list_ != NULL){
        for(auto &txn: *Txn_list_ ){
            Txn_list.push_back(txn);
        }
    }
    hash = get_hash();
}

string Block:: get_string(){
    ostringstream ss;
    ss << parent_id << timestamp;
    if(!Txn_list.empty()){
        for ( auto& txn: Txn_list) {
            ss << txn->get_string();
        }
    }
    return ss.str();
}

string Block::get_hash(){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    string block_string = get_string();
    SHA256(reinterpret_cast<const unsigned char *>(block_string.c_str()),block_string.size(),hash);
    // Convert to human readable string
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) <<setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int Block::size(){
    int size = 0;
    for (auto &txn: Txn_list){
        size += txn->size();
    }
    return size;
}

bool Txncomparator:: operator()(const Txn* a, const Txn* b) const{
    if(a->timestamp != b->timestamp) {
        return a->timestamp < b->timestamp;
    }
    else{
        return a->id < b->id;
    }
}

// updates the wallet and all transactions included in the chain

void chain::update_tail(Block* B){
    tail = B;
    depth += 1;
    for(Txn* T:B->Txn_list){
        if(T->payer_id == T->payee_id){
            wallet[T->payer_id] += T->amount;
        }
        else{
            added_txns.insert(T->id);
            wallet[T->payer_id] -= T->amount;
            wallet[T->payee_id] += T->amount;
        }
        added_txns.insert(T->id);
    }
}

chain:: chain(Block* B,chain* other){
    tail = B;
    depth = 1 + other->depth;
    wallet = other->wallet;
    for(int i: other->added_txns){
        added_txns.insert(i);
    }
    for(Txn* T:B->Txn_list){
        if(T->payer_id == T->payee_id){
            wallet[T->payer_id] += T->amount;
        }
        else{
            added_txns.insert(T->id);
            wallet[T->payer_id] -= T->amount;
            wallet[T->payee_id] += T->amount;
        }
        added_txns.insert(T->id);
    }
}

bool chaincomparator::operator()(const chain* a, const chain* b) const{
    if(a->depth != b->depth){
        return a->depth > b->depth;
    }else{
        return a->tail->id < b->tail->id;
    }
}

bool EventComparator:: operator()(const Event* a, const Event* b) const{
    if(a->timestamp != b->timestamp) {
        return a->timestamp < b->timestamp;
    }
    else{
        return a < b;
    }
}

bool Event::is_create_event(){
    return (type == CREATE_TXN || type == CREATE_BLK);
}

Link:: Link(int peer_id_,ld pij_, ld cij_){
    peer_id = peer_id_;
    pij = pij_;     // in seconds
    cij = cij_;     // in Mbps
    generate_dij = exponential_distribution<ld>(cij/0.09375);       // in seconds
}

ld Link :: get_delay(int msg_length){
    ld delay = pij + (ld(msg_length*8)/(cij*1024*1024)) + generate_dij(rng);   // in seconds
    return delay;
}

ostream& operator <<(ostream& out, const Txn& T) {
    out << "Time:[" << T.timestamp << "] Txn ID:[" << T.id <<"]";
    out << " [" << T.payer_id << "] Pays [" << T.payee_id <<"] Amount: " << T.amount << endl;
    return out;
}

ostream& operator <<(ostream& out, const Block& B) {
    out << "Block ID: [" << B.id << "] Parent ID: ["<< B.parent_id << "]";
    // for(auto &txn: B.Txn_list){
    //     out << txn->id << " ";
    // }
    out << " No of Txns [" << B.Txn_list.size() <<"]";
    return out;
}

ostream& operator <<(ostream& out, const Link& L){
    out << "pij - [" << L.pij <<"] " ;
    out << "Link speed -[" << L.cij << "]";
    return out; 
}

const char* eventTypeToString(EVENT_TYPE type) {
    switch (type) {
        case MINING_START: return "MINING_START";
        case CREATE_TXN: return "CREATE_TXN";
        case RECV_TXN: return "RECV_TXN";
        case CREATE_BLK: return "CREATE_BLK";
        case RECV_BLK: return "RECV_BLK";
        default: return "UNKNOWN";
    }
}

ostream& operator<<(ostream& os, const Event& event) {
    // os << "Time:[" << event.timestamp << "] ";
    os << "Event Type:[" << eventTypeToString(event.type) << "] Sender ID:[" << event.sender_id << "]";
    return os;
}

ostream& operator<<(ostream& os, const Event_TXN& event) {
    // os << "Time:[" << event.timestamp << "] ";
    os << "Event Type:[" << eventTypeToString(event.type) << "] Sender ID:[" << event.sender_id << "] Receiver ID:[" << event.receiver_id << "] Txn: " << *event.txn;
    return os;
}

ostream& operator<<(ostream& os, const Event_BLK& event) {
    // os << "Time:[" << event.timestamp << "] ";
    os << "Event Type:[" << eventTypeToString(event.type) << "] Sender ID:[" << event.sender_id << "] Receiver ID:[" << event.receiver_id << "] Block: " << *event.blk;
    return os;
}
