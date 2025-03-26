import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout
import matplotlib.pyplot as plt

def visualize_block_chain(file_path):
    G = nx.DiGraph() 
    # Read file and add edges to the graph
    with open(file_path, 'r') as f:
        for line in f:
            parent, child = line.strip().split(' ')
            G.add_edge(parent, child)
    
    pos = graphviz_layout(G, prog="dot")
    pos = {k: (-y, x) for k, (x, y) in pos.items()}
    fig = plt.figure(dpi=300)
    n = len(G)
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, 
        node_size=node_size, 
        font_size=font_size, 
        arrowsize=font_size, 
        width=edge_width, 
        node_color='darkred', 
        with_labels=True, 
        font_color="white"
    )
    plt.savefig("blockchain_30_30_6_600.png", bbox_inches='tight')
    plt.close()
    
    # Draw the graph

# Example usage
file_path = "blockchain.txt"  # Update with your actual file path
visualize_block_chain(file_path)
