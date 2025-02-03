#ifndef HELPER_H    // TO prevent multiple inclusion 
#define HELPER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <variant>
#include <openssl/sha.h>

struct Txn{

public :
    int sender_id, receiver_id;
    float amount, txn_time, txn_fee;

    Txn() : sender_id(0), receiver_id(0), amount(0), txn_time(0), txn_fee(0) {}

    Txn(float time,int sender, int receiver, float amt){
        sender_id = sender;
        receiver_id = receiver;
        amount = amt;
        txn_fee = 0;
        txn_time = time;
    }

    std::string get_string(){
        std::ostringstream ss;
        ss << txn_time << sender_id << receiver_id << amount;
        return ss.str();    
    }

    std::string get_hash(){
        unsigned char hash[SHA256_DIGEST_LENGTH];
        std::string txn_string = get_string();
        SHA256(reinterpret_cast<const unsigned char *>(txn_string.c_str()),txn_string.size(),hash);
        // Convert to human readable string
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex <<std:: setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
    int size(){ 
    // Size of the transaction assumed to be 1KB
        return 1000;
    }

};

struct Block{

public :
    std::string prev_block_hash, curr_block_hash;   // curr_block_hash works as blk_id
    float created_time;
    std::vector<Txn> Txn_list;

    Block() : prev_block_hash(""), curr_block_hash(""), created_time(0) {}

    std::string get_string(){
        std::ostringstream ss;
        ss << prev_block_hash << created_time;
        for ( auto& txn: Txn_list) {
            ss << txn.get_string();
        }
        return ss.str();
    }

    std::string get_hash(){
        unsigned char hash[SHA256_DIGEST_LENGTH];
        std::string block_string = get_string();
        SHA256(reinterpret_cast<const unsigned char *>(block_string.c_str()),block_string.size(),hash);
        // Convert to human readable string
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex <<std:: setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
};

typedef enum {
    NO_EVENT,
    CREATE_TXN,
    CREATE_BLK,
    RECV_TXN,
    RECV_BLK,
    SEND_TXN,
    SEND_BLK
} EVENT_TYPE;

struct Event {
    float event_time;
    EVENT_TYPE type;
    int event_creator, event_receiver;
    Txn txn_info;
    Block blk_info;    

    Event (float ev_time,EVENT_TYPE t, int creat, int recv, Txn T){
        event_time = ev_time;
        type = t; event_creator = creat;
        event_receiver = recv; txn_info = T;
    }
    Event (float ev_time,EVENT_TYPE t, int creat, int recv, Block B){
        event_time = ev_time;
        type = t; event_creator = creat;
        event_receiver = recv; blk_info = B;
    }
};

struct Eventqueue {

public:
    std::vector<Event> q;
    void push_event(Event E){
        if(is_empty()){
            q.push_back(E);
            return;
        }
        for(auto itr=q.begin();itr != q.end();itr++){
            if(E.event_time < itr->event_time){
                q.insert(itr,E);
                return;
            }
        }
        q.push_back(E);
        return;
    }
    void pop_event(){
        q.erase(q.begin());
    }
    Event top_event(){
        Event E = q[0];
        return E;
    }
    bool is_empty(){
        return q.empty();
    }
};


std::ostream& operator <<(std::ostream& out, const Txn& T) {
    out << "Time:[" << T.txn_time << "] Txn:[" << T.sender_id << "] Pays [" << T.receiver_id <<"] Amount: " << T.amount << std::endl;
    return out;
}

std::ostream& operator <<(std::ostream& out, const Block& B) {
    out << "Created Time:[" << B.created_time 
        << "] Block hash:[" <<B.curr_block_hash << "]" <<std::endl;
    for(const Txn &T: B.Txn_list){
        out << T;
    }
    return out;
}

#endif