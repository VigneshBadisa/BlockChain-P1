import os
import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout
import matplotlib.pyplot as plt


# Read the integers from blockchain.txt
with open("blockchain.txt", "r") as file:
    lines = file.readlines()

# Extract integers
integers = []
for line in lines:
    integers.extend(map(int, line.strip().split(',')))

# Directory containing the log files
log_dir = "logs"

names = ["honest1", "honest2", "Ringmaster","Mal1"]
# Open and read each Peer<n>.log file
for num,name in zip(integers,names):
    file_path = os.path.join(log_dir, f"Peer{num}.log")
    
    try:
        G = nx.DiGraph() 
        # Read file and add edges to the graph
        with open(file_path, 'r') as f:
            lines = f.readlines()
        mined = {}
        for line in lines:
            if line.startswith("Block ID"):
                parts = line.strip().split(", ")
                block_id = int(parts[0].split(": ")[1])
                parent_id = int(parts[1].split(": ")[1])
                miner_id = int(parts[2].split(": ")[1])
                # print(block_id,parent_id,miner_id)
                if(block_id == 0) :
                    continue
                if(parent_id == 0) :
                    parent_id = "Genesis"
                mined[block_id] = miner_id
                G.add_edge(parent_id,block_id)
    
        pos = graphviz_layout(G, prog="dot")
        pos = {k: (-y, x) for k, (x, y) in pos.items()}
        fig = plt.figure(dpi=300)
        n = len(G)
        node_size = 7000 // n
        font_size = min(8, 700 // n)
        edge_width = min(1.0, font_size / 4)
        colors = []
        for node in G.nodes():
            miner_id = mined.get(node, None)  # Use .get() to avoid KeyError

            if node == "Genesis":
                colors.append('darkblue')
            elif miner_id is None:
                colors.append('gray')  # Default color for unknown nodes
            elif miner_id == integers[2]:
                colors.append('darkred')
            else:
                colors.append('lightgreen')

        nx.draw(G, pos, 
            node_size=node_size, 
            font_size=font_size, 
            arrowsize=font_size, 
            width=edge_width, 
            node_color=colors, 
            with_labels=True, 
            font_color="white"
        )
        plt.savefig(f"blockchain_{name}.png", bbox_inches='tight')
        plt.close()
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
