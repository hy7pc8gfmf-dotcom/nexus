#!/usr/bin/env python3
"""
port_seed_bank.py — 从 D:/synapse 移植种子库到 D:/nexus

读取 .unified_seed_bank.json (32,629 种子, 184 域)
输出到 .nexus/seed_bank.jsonl (每行一颗种子, 方便 C++ 逐行读取)
"""

import json
import os
import sys
import time

SYNAPSE_BANK = r"D:/synapse/logic_solve_engine/.unified_seed_bank.json"
SYNAPSE_STATE = r"D:/synapse/logic_solve_engine/.engine_state.json"
NEXUS_DIR = r"D:/nexus/.nexus"
NEXUS_BANK = r"D:/nexus/.nexus/seed_bank.jsonl"
NEXUS_INDEX = r"D:/nexus/.nexus/seed_bank_index.json"

def main():
    os.makedirs(NEXUS_DIR, exist_ok=True)

    t0 = time.time()

    # 1. 读取种子库
    print(f"读取种子库: {SYNAPSE_BANK}", flush=True)
    with open(SYNAPSE_BANK, 'r', encoding='utf-8') as f:
        data = json.load(f)

    seeds = data.get('seeds', {})
    domain_index = data.get('domain_index', {})
    metadata = data.get('metadata', {})
    version = data.get('version', 2)
    total = data.get('total_seeds', len(seeds))
    dimension = metadata.get('dimension', 14)

    print(f"  种子: {len(seeds)}, 域: {len(domain_index)}, 维度: {dimension}", flush=True)

    # 2. 按域索引构建域→种子名映射
    domain_to_seeds = {}
    for domain, entries in domain_index.items():
        names = [e['name'] for e in entries if 'name' in e]
        domain_to_seeds[domain] = names

    # 3. 写入 JSONL (每行一颗种子)
    print(f"写入 JSONL: {NEXUS_BANK}", flush=True)
    written = 0
    with open(NEXUS_BANK, 'w', encoding='utf-8') as out:
        for name, seed in seeds.items():
            # 查找域
            domains = []
            for d, names in domain_to_seeds.items():
                if name in names:
                    domains.append(d)

            record = {
                "name": name,
                "intensity": seed.get("intensity", 0),
                "source": seed.get("source", ""),
                "type": seed.get("type", ""),
                "domain_tag": seed.get("domain_tag", ""),
                "domains": domains,
                "step_detail": seed.get("step_detail", ""),
            }
            out.write(json.dumps(record, ensure_ascii=False) + "\n")
            written += 1

    # 4. 写入索引 (快速域查询)
    print(f"写入索引: {NEXUS_INDEX}", flush=True)
    index = {
        "version": version,
        "total_seeds": total,
        "dimension": dimension,
        "domains": list(domain_index.keys()),
        "domain_counts": {d: len(v) for d, v in domain_to_seeds.items()},
    }
    with open(NEXUS_INDEX, 'w', encoding='utf-8') as f:
        json.dump(index, f, ensure_ascii=False, indent=2)

    elapsed = time.time() - t0
    print(f"完成: {written} 种子, {len(domain_index)} 域, {elapsed:.1f}s", flush=True)

if __name__ == "__main__":
    main()
