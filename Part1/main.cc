#include "simulator.h"
#include "node.h"
#include <iostream>
#include <getopt.h>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[]) {
    int n = -1;  // Set to invalid values initially
    ld z0 = -1, z1 = -1, T_tx = -1, Tk = -1, simEndtime = -1;

    static struct option long_options[] = {
        {"n", required_argument, 0, 'n'},
        {"z0", required_argument, 0, 'z'},
        {"z1", required_argument, 0, 'Z'},
        {"Ttx", required_argument, 0, 'T'},
        {"Tk", required_argument, 0, 'K'},
        {"t", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "n:z:Z:T:K:s:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'n': n = stoi(optarg); break;
            case 'z': z0 = stod(optarg); break;
            case 'Z': z1 = stod(optarg); break;
            case 'T': T_tx = stod(optarg); break;
            case 'K': Tk = stod(optarg); break;
            case 's': simEndtime = stod(optarg); break;
            default:
                cerr << "Usage: " << argv[0] << " --n <nodes> --z0 <z0> --z1 <z1> --Ttx <T_tx> --Tk <Tk> --t <simEndtime>\n";
                return 1;
        }
    }

    if (n == -1 || z0 == -1 || z1 == -1 || T_tx == -1 || Tk == -1 || simEndtime == -1) {
        cerr << "Error: Missing required arguments!\n";
        cerr << "Usage: " << argv[0] << " --n <nodes> --z0 <z0> --z1 <z1> --Ttx <T_tx> --Tk <Tk> --t <simEndtime>\n";
        return 1;
    }

    simulator sim(n,z0,z1,simEndtime,T_tx,Tk);
    sim.start();
    sim.write_tree_file();
    // deleting all the pointers
    for(int i=0;i<n;i++){
        delete sim.nodes[i];
    }
    sim.nodes.clear();
    delete sim.GENESIS_blk;
    delete sim.top_event;

    return 0;
}
