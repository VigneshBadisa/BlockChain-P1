#include <iostream>

class node {
public:
    virtual void recv_blk() { // Virtual function ensures dynamic dispatch
        std::cout << "recv_blk from node\n";
    }
    virtual ~node() {} // Virtual destructor for proper cleanup
};

class malnode : public node {
public:
    void recv_blk() override { // Override base class function
        std::cout << "recv_blk from malnode\n";
    }
};

int main() {
    node* x = new malnode(); // Base class pointer, derived class object
    x->recv_blk(); // Calls malnode::recv_blk() because of virtual function
    delete x; // Cleanup
    return 0;
}
