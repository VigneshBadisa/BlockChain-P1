#!/usr/bin/env python3
import os
import sys
import argparse
import matplotlib
matplotlib.use('Agg')
from matplotlib import style
import matplotlib.pyplot as pplot
import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout
import pandas as pd
import numpy as np

def GetEdges(edgelist_file):
    edges = []
    with open(edgelist_file, 'r') as f:
        for line in f.readlines():
            if not line.strip():
                continue
            edge_info = line.strip().split()
            edges.append((int(edge_info[0]), int(edge_info[1])))

    return edges

def GenerateBlockChainGraph(G, output_file):
    # Use 'dot' layout for this graph
    pos = graphviz_layout(G, prog="dot")

    pos = {k: (-y, x) for k, (x, y) in pos.items()}
    fig = pplot.figure(dpi=300)
    node_size = 7000 //len(G)
    font_size = min(8, 700 // len(G))
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, node_size=node_size, font_size=font_size, arrowsize=font_size, width=edge_width, node_color='darkred', with_labels=True, font_color="white")

    # Finally draw the blockchain graph
    pplot.savefig(output_file, bbox_inches='tight')
    pplot.close()


def GenerateNetworkGraph(network_file, output_file, distinguish_adversary_nodes=True):
    # First read the network_file and create the edge infos
    edges = GetEdges(network_file)

    # Now create the graph from the edge list
    G = nx.Graph(edges)

    adversary_node_idx = max(G.nodes()) if distinguish_adversary_nodes else -1
    colours = [('darkred' if v == adversary_node_idx else 'blue') for v in G.nodes()]

    # Use 'circular' layout for this graph
    pos = graphviz_layout(G, prog='circo')

    # Now generate the network graph
    nx.draw(G, pos, node_color=colours, with_labels=True, font_color="white")

    # Finally draw the network graph
    pplot.savefig(output_file, bbox_inches='tight')
    pplot.close()

def R_pool(alpha, gamma):
    numerator = (alpha * (1 - alpha) * (1 - alpha) * (4 * alpha + gamma * (1 - 2 * alpha)) - alpha * alpha * alpha)
    denominator = 1 - alpha * (1 + alpha * (2 - alpha))
    return numerator / denominator

def main():
    parser = argparse.ArgumentParser(
            prog = "P2P-CryptoCurrency-Network-Visualizer",
            description = "Create visual graphs from simulator's output",
            formatter_class=argparse.ArgumentDefaultsHelpFormatter
        )

    parser.add_argument('-p', '--peer_index', default=0, help="Provide the peer index for which you want to dump the simulation points")
    parser.add_argument('-o', '--output_dir', default=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'output'), help="Provide the output directory path")

    args = parser.parse_args()

    BASE_OUTDIR = os.path.abspath(args.output_dir)

    # First dump the network graph
    GenerateNetworkGraph(os.path.join(BASE_OUTDIR, 'peer_network_edgelist.txt'), os.path.join(BASE_OUTDIR, 'peer_network_img.png'))

    num_peers = len(os.listdir(os.path.join(BASE_OUTDIR, 'block_arrivals')))
    peer_idx = (args.peer_index % num_peers + num_peers) % num_peers
    peer_file = f'Peer{peer_idx + 1}.txt'
    adversary = num_peers

    for folder in ['final_blockchains', 'termination_blockchains']:
        path = os.path.join(BASE_OUTDIR, folder, peer_file)
        edges = GetEdges(path)
        G = nx.DiGraph(edges)
        filename = path[:-4] + '_img.png'
        GenerateBlockChainGraph(G, filename)

    stat_file = os.path.join(BASE_OUTDIR, 'peer_stats', peer_file)
    df = pd.read_csv(stat_file, index_col='id')

    total_blocks = df['generated_blocks'].sum()
    total_blocks_in_chain = df['chain_blocks'].sum()
    total_hash_power = df['hash_power'].sum()
    adversary_stats = df.loc[adversary].copy()
    alpha = round(adversary_stats['hash_power'] / total_hash_power, 2)

    mpu_adv = adversary_stats['chain_blocks'] / adversary_stats['generated_blocks']
    rpool = adversary_stats['chain_blocks'] / total_blocks_in_chain
    mpu_overall = total_blocks_in_chain / total_blocks

    print(f'--- Alpha = {alpha:.2f}')
    print(f'--- MPU_adv = {mpu_adv:.5f}')
    print(f'--- MPU_overall = {mpu_overall:.5f}')
    print(f'--- Gamma_0 = {R_pool(alpha, 0):.5f}')
    print(f'--- Gamma_1 = {R_pool(alpha, 1):.5f}')
    print(f'--- R_pool = {rpool:.5f}')

    df = df.sort_values(by='hash_power').reset_index(drop=True)
    fraction_blocks_in_chain = df['chain_blocks'].to_numpy() / total_blocks_in_chain
    peer_ids = df.index.to_numpy()
    slow_peers = df[df['is_fast'] == 0]
    fast_peers = df[df['is_fast'] == 1]

    stat_outfile = stat_file[:-4] + '_stats.png'
    style.use('ggplot')
    fig = pplot.figure(dpi=300)
    pplot.xlabel('Peer ID')
    pplot.xticks(peer_ids, [str(o + 1) if o % 5 == 0 or o + 1 == len(peer_ids) else '' for o in peer_ids])
    pplot.title('Statistics')
    pplot.plot(peer_ids, df['hash_power'].to_numpy() / total_hash_power, label='Fraction of Hash Power')
    pplot.scatter(slow_peers.index, fraction_blocks_in_chain[slow_peers.index], label='Fraction of Blocks in Longest Chain, Slow Peer')
    pplot.scatter(fast_peers.index, fraction_blocks_in_chain[fast_peers.index], label='Fraction of Blocks in Longest Chain, Fast Peer')
    pplot.legend(prop={'size': 8}, framealpha=0.3, loc='upper left')
    pplot.savefig(stat_outfile, bbox_inches='tight')
    pplot.close()

if (__name__=='__main__'):
    main()
