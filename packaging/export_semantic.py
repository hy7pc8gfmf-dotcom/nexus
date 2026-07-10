#!/usr/bin/env python3
"""
export_semantic.py — 导出语义场数据为 C++ 二进制格式

从 Python 的 .semantic_slime_max.pkl 导出到 .semantic_field.bin
供 C++ SemanticPort::load() 加载。

用法:
  python export_semantic.py [--input <pkl_path>] [--output <bin_path>]
"""

import struct
import sys
import os
import pickle

# 添加 D:/synapse 到路径
_SELF = os.path.dirname(os.path.abspath(__file__))
_SYNAPSE = os.path.join(_SELF, '..', '..', '..', 'synapse')
_D_SYNAPSE = r'D:\synapse'
for p in [_SYNAPSE, _D_SYNAPSE]:
    if p not in sys.path and os.path.isdir(p):
        sys.path.insert(0, p)

K_SEMANTIC_DIM = 14

def export_semantic(pkl_path, bin_path):
    """导出语义场数据"""
    print(f"加载: {pkl_path}")

    # 直接从 pickle 加载裸数据 (绕过 SemanticPort)
    with open(pkl_path, 'rb') as f:
        data = pickle.load(f)

    hotspots = data.get('hotspots', {})
    pipes = data.get('pipes', {})

    print(f"热点: {len(hotspots)}")
    print(f"管道: {len(pipes)}")

    # 构建名称→索引映射
    names = list(hotspots.keys())
    n_concept = len(names)
    name_to_idx = {name: i for i, name in enumerate(names)}

    # 构建邻接表
    adj = [[] for _ in range(n_concept)]
    total_edges = 0
    for key, val in pipes.items():
        parts = key.split('|')
        if len(parts) != 2: continue
        src, dst = parts
        if src in name_to_idx and dst in name_to_idx:
            si = name_to_idx[src]
            di = name_to_idx[dst]
            if di not in adj[si]:
                adj[si].append(di)
                total_edges += 1

    print(f"边: {total_edges}")

    # 写入二进制
    with open(bin_path, 'wb') as f:
        # HEADER
        f.write(struct.pack('<IIIII',
            0x4D45534E,  # magic "NSEM"
            1,           # version
            n_concept,   # n_concept
            total_edges, # n_edge
            K_SEMANTIC_DIM))  # dim

        # CONCEPTS
        for name in names:
            entry = hotspots[name]

            # coord (14 float32) — 热点数据就是 14D 坐标列表
            coord = entry
            if isinstance(coord, dict):
                coord = coord.get('coord', [0.0] * K_SEMANTIC_DIM)
            if not isinstance(coord, (list, tuple)) or len(coord) < K_SEMANTIC_DIM:
                coord = [0.0] * K_SEMANTIC_DIM
            coord = [float(v) for v in coord[:K_SEMANTIC_DIM]]

            # intensity (当 entry 是 list 时用默认值)
            intensity = 0.5
            lang = 'zh'
            if isinstance(entry, dict):
                intensity = float(entry.get('intensity', entry.get('weight', 0.5)))
                lang = entry.get('lang', 'zh')
            name_bytes = name.encode('utf-8')
            lang_bytes = lang.encode('utf-8')

            # write: name_len(2) + name + coord(14*4) + intensity(4) + lang_len(2) + lang + pad(2)
            f.write(struct.pack('<H', len(name_bytes)))
            f.write(name_bytes)
            for v in coord:
                f.write(struct.pack('<f', float(v)))
            f.write(struct.pack('<f', intensity))
            f.write(struct.pack('<H', len(lang_bytes)))
            if lang_bytes:
                f.write(lang_bytes)
            f.write(b'\x00\x00')  # padding

        # EDGES (邻接表)
        for i in range(n_concept):
            neighbors = adj[i]
            f.write(struct.pack('<I', len(neighbors)))
            for ni in neighbors:
                f.write(struct.pack('<i', ni))

    size_mb = os.path.getsize(bin_path) / 1024 / 1024
    print(f"导出: {bin_path} ({size_mb:.1f} MB)")


if __name__ == '__main__':
    # 默认路径
    default_pkl = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        '..', '..', '..', 'synapse', '.semantic_slime_max.pkl')

    default_bin = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        '..', 'binary', '.semantic_field.bin')

    pkl_path = sys.argv[2] if len(sys.argv) > 2 and sys.argv[1] == '--input' else default_pkl
    bin_path = sys.argv[4] if len(sys.argv) > 4 and sys.argv[3] == '--output' else default_bin

    # 规范化路径
    pkl_path = os.path.abspath(pkl_path)
    bin_path = os.path.abspath(bin_path)
    os.makedirs(os.path.dirname(bin_path), exist_ok=True)

    export_semantic(pkl_path, bin_path)
