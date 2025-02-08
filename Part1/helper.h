#ifndef HELPER_H    // TO prevent multiple inclusion 
#define HELPER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <variant>
#include <ctime>

typedef long double ld;

struct Txn{

public :
    const int id;
    ld timestamp;
    int payer_id, payee_id, amount;

    Txn(int id_,ld timestamp_ ,int payer_id_, int payee_id_, int amount_);
    ~Txn() = default;
    std::string get_string();
    std::string get_hash();
    int size();

};

struct Block{

public :
    int id;
    std::string parent_hash, hash;   // curr_block_hash works as blk_id
    ld timestamp;
    std::vector<Txn*> Txn_list;

    // Block() : prev_block_hash(""), curr_block_hash(""), timestamp(0) {}

    Block(int id_,ld timestamp_ ,std::string parent_hash_, std::vector<Txn*>*Txn_list_);

    ~Block() = default;
    std::string get_string();
    std::string get_hash();
    int size();

};

typedef enum {
    EMPTY,
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

    virtual ~Event() = default;
};

struct EventComparator {
    bool operator()(const Event* a, const Event* b);
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


std::ostream& operator<<(std::ostream& os, const Txn& txn);
std::ostream& operator<<(std::ostream& os, const Block& block);

#endif