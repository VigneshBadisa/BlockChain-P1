#include "helper.h"

#include <sstream>
#include <openssl/sha.h>


using namespace std;

Txn::Txn(ld timestamp_ ,int payer_id_, int payee_id_, int amount_){
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

Block::Block(ld timestamp_ ,::string parent_hash_, ::vector<Txn*>*Txn_list_){
    timestamp = timestamp_;
    parent_hash = parent_hash_;
    for(auto &txn: *Txn_list_ ){
        Txn_list.push_back(txn);
    }
    hash = get_hash();
}

string Block:: get_string(){
    ostringstream ss;
    ss << parent_hash << timestamp;
    for ( auto& txn: Txn_list) {
        ss << txn->get_string();
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

ostream& operator <<(ostream& out, const Txn& T) {
    out << "Time:[" << T.timestamp << "] Txn:[" << T.payer_id << "] Pays [" << T.payee_id <<"] Amount: " << T.amount << endl;
    return out;
}

ostream& operator <<(ostream& out, const Block& B) {
    out << "Created Time:[" << B.timestamp
        << "] Block hash:[" << B.hash << "]" << endl;
    for(auto &txn: B.Txn_list){
        out << *txn;
    }
    return out;
}