#include "node.h"
#include "simulator.h"
#include <queue>
#include <assert.h>

using namespace std;

MalNode::MalNode(int id_, bool is_slow_ ,bool is_highhash_,bool eclipse_attack_, bool is_ringmaster_,ld Tt_, ld Ttx_,ld Tk_, ld hash_power_ , Block* genesis_blk_){
    id = id_;
    is_slow = is_slow_;
    is_highhash = is_highhash_;
    is_ringmaster = is_ringmaster_;
    eclipse_attack = eclipse_attack_;
    if(eclipse_attack)is_attacking = true;
    else is_attacking = false;
    Tt = Tt_,
    Ttx = Ttx_;
    Tk = Tk_;
    hash_power = hash_power_;
    genesis_blk = genesis_blk_;
    generate_Ttx = exponential_distribution<ld>(1/Ttx);
    if(is_ringmaster) generate_tk = exponential_distribution<ld>(hash_power/Tk);
    select_rand_real = uniform_real_distribution<ld>(0,1);
}

void MalNode:: send_private_blk(int peer_id, Block* B, simulator* simul){
    int msg_len = B->size();
    ld latency_ij = mallatency[peer_id]->get_delay(msg_len);
    Event_BLK* E = new Event_BLK(latency_ij,RECV_MAL_BLK,id,peer_id,B);
    simul->add_event(E);
}

Block* MalNode:: create_blk(simulator* simul){
    int parent_id = private_chain->tail->id;
    vector<Txn*> txn_list;
    Txn* T = new Txn(simul->simclock,id,id,50); // Coinbase Txn
    txn_list.push_back(T);
    int size = 1;
    vector<int> temp_wallet(private_chain->wallet);
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

void MalNode:: mining_success(Block* B,simulator* simul,bool stop_mining){
    // longest_chain = *tail_blks.begin();
    recvd_time[B->id] = simul->simclock;
    if(B->id == mining_blk->id && B->parent_id == private_chain->tail->id){
        numMinedblks += 1;
        simul->total_mined_blks += 1;
        cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - ";
        cout<<"Mining Success : ";
        cout<<*B<<endl;
        AllBlks[B->id] = B;
        for(int i:adj_malpeers) send_private_blk(i,B,simul);
        private_chain->update_tail(B);
        children[B->parent_id].push_back(B->id);
        for(Txn* T: B->Txn_list){
            txn_pool.erase(T);
        }
        if(!stop_mining) mine_new_blk(simul);
    }
}

void MalNode::selfish_mining(bool stop_mining, simulator* simul){
    if(is_ringmaster){
        longest_chain = *tail_blks.begin();
        int branch_chain_depth = private_chain->depth;
        Block* B = private_chain->tail;
        while(B->id != branch_blk->id){
            branch_chain_depth -= 1;
            B = AllBlks[B->parent_id];
        }
        int new_pchain_depth = private_chain->depth - branch_chain_depth;
        int new_hchain_depth = longest_chain->depth - branch_chain_depth;
        
        cout <<"[" << simul->simclock << "] MalNode :[" << id  << "]  Hdepth :[" << new_hchain_depth  << "] Pdepth: [" << new_pchain_depth << "]" <<endl;

        assert(new_hchain_depth >= 0);

        if(new_hchain_depth > 0){
            if(new_hchain_depth > new_pchain_depth ){
                assert(new_pchain_depth == 0);
                if(!stop_mining)cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] Mining on New Longest honest chain " <<endl;
                private_chain = new chain(longest_chain);
                branch_blk = private_chain->tail;
                if(!stop_mining) mine_new_blk(simul);
            }else if(new_hchain_depth > new_pchain_depth - 2){
                // cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - ";
                // cout <<"Broadcasting private chain from : [" << branch_blk->id << "] to Block [" << private_chain->tail->id <<"]" <<endl;
                broadcast_private_chain(simul);
            }
        }else{
            cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - No new Honest Chain found " <<endl;
        }
    }
    return ;
}

void MalNode:: recv_get_req(int peer_id,Block_Hash* bhash, simulator* simul){
    if(!is_attacking){
        Block* B = AllBlks[bhash->id];
        send_blk(peer_id,B,simul);
    }
}

void MalNode::recv_hash(int peer_id,Block_Hash* bhash, simulator* simul){
    if(AllHashes.find(bhash->id) != AllHashes.end()){
        if(AllBlks.find(bhash->id) != AllBlks.end()) return;
        int msg_len = bhash->size();
        ld latency_ij = latency[peer_id]->get_delay(msg_len);
        if(simul->simclock - Hashtimeout[id] > Tt){            
            Event_HASH* E = new Event_HASH(latency_ij,GET_REQ,id,peer_id,bhash);
            Hashtimeout[id] = simul->simclock;
            simul->add_event(E);
        }else{
            ld delay = Hashtimeout[id] + Tt - simul->simclock + latency_ij;
            Event_HASH* E = new Event_HASH(delay,GET_REQ,id,peer_id,bhash);
            Hashtimeout[id] = simul->simclock;
            simul->add_event(E);    
        }
    }else{
        if(AllBlks.find(bhash->id) != AllBlks.end()){
            // cout << "Block without Hash" << endl;
            AllHashes[bhash->id] = bhash;
            return;
        }
        if(eclipse_attack) is_attacking = true;
        AllHashes[bhash->id] = bhash;
        int msg_len = bhash->size();
        ld latency_ij = latency[peer_id]->get_delay(msg_len);
        Event_HASH* E = new Event_HASH(latency_ij,GET_REQ,id,peer_id,bhash);
        Hashtimeout[bhash->id] = simul->simclock;
        simul->add_event(E);
    }
}

void MalNode :: recv_hon_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) {
    // cout << "Node :[" << id  << "] Created Events" <<endl;
    if(AllBlks.find(B->id) != AllBlks.end()) return;    // Block already received
    AllBlks[B->id] = B;
    for(int i:adj_malpeers){
        if(i == peer_id) continue;
        int msg_len = B->size();
        ld latency_ij = mallatency[i]->get_delay(msg_len);
        Event_BLK* E = new Event_BLK(latency_ij,RECV_HON_BLK,id,i,B);
        simul->add_event(E);
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
        selfish_mining(stop_mining,simul);
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
            selfish_mining(stop_mining,simul);
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
    selfish_mining(stop_mining,simul);   
}

void MalNode:: recv_blk(int peer_id, Block* B, simulator* simul,bool stop_mining){

    // cout << "Node :[" << id  << "] Created Events" <<endl;
    // cout << "H received" <<endl;
    if(AllBlks.find(B->id) != AllBlks.end()) return;    // Block already received
    AllBlks[B->id] = B;
    for(int i:adj_peers){
        if(peer_id == i) continue;
        send_hash(i,AllHashes[B->id],simul);
    }
    for(int i:adj_malpeers){
        int msg_len = B->size();
        ld latency_ij = mallatency[i]->get_delay(msg_len);
        Event_BLK* E = new Event_BLK(latency_ij,RECV_HON_BLK,id,i,B);
        simul->add_event(E);
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
        selfish_mining(stop_mining,simul);
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
            selfish_mining(stop_mining,simul);
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
    selfish_mining(stop_mining,simul);   
}

void MalNode:: broadcast_private_chain(simulator* simul){
    if(branch_blk->id == private_chain->tail->id) {
        // cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - Already broadcasted or No private chain" <<endl;
        return;
    }
    if(is_ringmaster){
        cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - ";
        cout <<"Broadcasting private chain from : [" << branch_blk->id << "] to Block [" << private_chain->tail->id <<"]" <<endl;    
    }
    if(eclipse_attack) is_attacking = false;
    // cout << "check 1" <<endl;
    for(int peer_id: adj_malpeers){
        int msg_len = 64;
        ld latency_ij = mallatency[peer_id]->get_delay(msg_len);
        Event* E = new Event(latency_ij,BC_PRIV_CHAIN,peer_id);
        simul->add_event(E);
    }
    // cout << "check 2" <<endl;
    Block* B = private_chain->tail;
    while(B->id != branch_blk->id){
        // cout << "Sending hash to Honest network - Hash :" << B->id  <<endl;
        Block_Hash* bhash = new Block_Hash(B->id,B->hash);
        AllHashes[B->id] = bhash;
        for(int i:adj_peers){
            send_hash(i,bhash,simul);
            // send_blk(i,B,simul);
        }
        B = AllBlks[B->parent_id];
    }
    // cout << "check 3" <<endl;
    tail_blks.insert(private_chain);
    private_chain = new chain(private_chain);
    branch_blk = private_chain->tail;
    // cout <<"[" << simul->simclock << "] MalNode :[" << id  << "] - Broadcasted Private chain till [" << branch_blk->id <<"]"<<endl;
}

void  MalNode:: recv_mal_blk(int peer_id, Block* B, simulator* simul,bool stop_mining) {

    
    if(AllBlks.find(B->id) != AllBlks.end()) return;    // Block already received
    AllBlks[B->id] = B;
    for(int i:adj_malpeers){
        if(peer_id == i) continue;
        send_private_blk(i,B,simul);
    }
    recvd_time[B->id]= simul->simclock;

    if(B->parent_id == private_chain->tail->id) {
        if(!is_blk_valid(B,private_chain)) {
            cout<<"[" << simul->simclock << "] Node :[" << id  << "] 1:Invalid" << *B <<endl;
            return;
        }
        private_chain->update_tail(B);
        children[B->parent_id].push_back(B->id);
        return;
    }

    if(AllBlks.find(B->parent_id) == AllBlks.end()){
        cout << "[" << simul->simclock << "] MalNode :[" << id  << "] Orphan Blk :["<< B->id << "], Parent ID:[" << B->parent_id << "] found " <<endl;
        exit(1);
        return;
    }

    // cout<<"[" << simul->simclock << "] MalNode :[" << id  << "] Creating new Private chain" <<endl ;

    assert(private_chain->tail->id == branch_blk->id);
    private_chain = create_new_chain(B,simul);

    if (private_chain == nullptr){
        cout << "[" << simul->simclock << "] MalNode :[" << id  << "] Orphan chain found " <<endl;
        exit(1);
        return;
    }

    branch_blk = AllBlks[B->parent_id];
    children[B->parent_id].push_back(B->id);

}
