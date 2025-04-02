#include "simulator.h"
#include "node.h"
#include <iostream>
#include <getopt.h>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[]) {
    int n = -1, e = -1;  // Set to invalid values initially
    ld  m = -1,Tt = -1, T_tx = -1, Tk = -1, simEndtime = -1;

    static struct option long_options[] = {
        {"n", required_argument, 0, 'n'},
        {"m", required_argument,0,'m'},
        {"Tt", required_argument,0,'t'},
        {"Ttx", required_argument, 0, 'T'},
        {"Tk", required_argument, 0, 'K'},
        {"t", required_argument, 0, 's'},
        {"e", required_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "n:m:t:T:K:s:e:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'n': n = stoi(optarg); break;
            case 'm': m = stod(optarg); break;
            case 't': Tt = stod(optarg); break;
            case 'T': T_tx = stod(optarg); break;
            case 'K': Tk = stod(optarg); break;
            case 's': simEndtime = stod(optarg); break;
            case 'e': e = stoi(optarg); break;
            default:
                cerr << "Usage: " << argv[0] << " --n <nodes> --m <m> --Tt <Tt> --Ttx <T_tx> --Tk <Tk> --t <simEndtime> --e <eclipse(0 or 1)>\n";
                return 1;
        }
    }

    if (n == -1||m == -1 || T_tx == -1 || Tk == -1 || simEndtime == -1) {
        cerr << "Error: Missing required arguments!\n";
        cerr << "Usage: " << argv[0] << " --n <nodes> --m <m> --Tt <Tt> --Ttx <T_tx> --Tk <Tk> --t <simEndtime> --e <eclipse(0 or 1)\n";
        return 1;
    }

    bool eclipse_attack;
    if(e == 0) eclipse_attack = false;
    else eclipse_attack = true;


    simulator sim(n,m,simEndtime,Tt,T_tx,Tk,eclipse_attack);
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
