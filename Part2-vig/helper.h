#ifndef HELPER_H    // TO prevent multiple inclusion 
#define HELPER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <variant>
#include <ctime>
#include <unordered_set>
#include <random>

extern std::mt19937_64 rng;
typedef long double ld;

struct simulator;

struct Txn{

public :
    static int counter;
    int id;
    ld timestamp;
    int payer_id, payee_id, amount;

    Txn(ld timestamp_ ,int payer_id_, int payee_id_, int amount_);
    ~Txn() = default;
    std::string get_string();
    std::string get_hash();
    int size();

};

struct Txncomparator{
    bool operator()(const Txn* a,const Txn*  b) const;
};

struct Block{

public :
    static int counter;
    ld timestamp;
    int id, parent_id;
    std::string hash;   // curr_block_hash works as blk_id
    std::vector<Txn*> Txn_list;

    // Block() : prev_block_hash(""), curr_block_hash(""), timestamp(0) {}

    Block(ld timestamp_ ,int parent_id_,std::vector<Txn*>*Txn_list_);

    ~Block() = default;
    std::string get_string();
    std::string get_hash();
    int size();

};

struct chain{

public:
    Block* tail;
    int depth;
    std::vector<int> wallet;
    std::unordered_set<int> added_txns;
    chain(Block* tail_,int depth_,int n,int amount):
        tail(tail_),depth(depth_),wallet(n,amount) {}
    chain(Block* tail, chain* other);
    void update_tail(Block* B);
    // ~chain() = default;
};

struct chaincomparator{
    bool operator()(const chain* a, const chain* b) const ;
};

typedef enum {
    MINING_START,
    CREATE_TXN,
    RECV_TXN,
    CREATE_BLK,
    RECV_BLK
} EVENT_TYPE;


class Event {

public :
    ld timestamp;
    EVENT_TYPE type;
    int sender_id;

    Event(ld timestamp_, EVENT_TYPE type_,int sender_id_ )
        : timestamp(timestamp_), type(type_), sender_id(sender_id_) {}
    
    bool is_create_event();
    
    virtual ~Event() = default;
};

struct EventComparator {
    bool operator()(const Event* a, const Event* b) const;
};

class Event_TXN : public Event {

public:
    int receiver_id;
    Txn* txn;
    Event_TXN(ld timestamp_, EVENT_TYPE type_ ,int sender_id_, int receiver_id_, Txn* txn_)
        :Event(timestamp_,type_,sender_id_),receiver_id(receiver_id_) ,txn(txn_) {}
};

class Event_BLK : public Event {

public:
    int receiver_id;
    Block* blk;
    Event_BLK(ld timestamp_, EVENT_TYPE type_,int sender_id_, int receiver_id_ , Block* blk_)
        :Event(timestamp_,type_,sender_id_),receiver_id(receiver_id_), blk(blk_) {}
};

class Link{
public:
    int peer_id;
    ld pij, cij;
    std::exponential_distribution<ld> generate_dij;

    Link(int peer_id_,ld pj_, ld cij_);
    ld get_delay(int msg_length);
};

struct stats{
public:

};

const char* eventTypeToString(EVENT_TYPE type);

std::ostream& operator<<(std::ostream& os, const Txn& txn);
std::ostream& operator<<(std::ostream& os, const Block& block);
std::ostream& operator<<(std::ostream& os, const Link& L);
std::ostream& operator<<(std::ostream& os, const Event& event);
std::ostream& operator<<(std::ostream& os, const Event_TXN& event);
std::ostream& operator<<(std::ostream& os, const Event_BLK& event) ;

#endif