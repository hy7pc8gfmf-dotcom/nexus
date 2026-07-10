#!/usr/bin/env python3
"""
index_proofs.py — 构造性证明库索引器

扫描 D:/proof_libraries/, 提取定理名, 甄别构造性,
输出为 C++ 可加载的索引 JSON。

用法:
  python index_proofs.py [--output <path>]
"""

import os, re, json, sys, argparse
from pathlib import Path

# ── 经典/非构造性公理签名库 ──
CLASSICAL_AXIOMS = {
    'Classical_Prop', 'classical_Prop', 'classical_left', 'classical_right',
    'excluded_middle', 'EM', 'NNPP', 'classic_dec', 'prop_dec',
    'classical_description', 'classical_Type', 'classical_double_theorem',
    'classical_excluded_middle', 'classical_or_not', 'classical_not_or',
    'classical_forall_exists_inertia', 'classical_not_exists_inertia',
    'Choice', 'choice', 'rel_choice', 'subset_types',
    'ClassicalDescription', 'classical_bicartesian_law',
    'classical_dependent_description', 'dependent_description',
    'epsilon_statement', 'i_epsilon_statement', 'epsilon_some',
    'indefinite_description', 'constructive_definite_description',
    'founded_choice', 'guarded_rel_choice', 'guarded_founded_choice',
}

# ── 已知纯构造性模块 (免检) ──
CONSTRUCTIVE_MODULES = {
    'Coq.Init', 'Coq.Unicode', 'Coq.Arith', 'Coq.NArith', 'Coq.ZArith',
    'Coq.QArith', 'Coq.Lists', 'Coq.Bool', 'Coq.Strings', 'Coq.Vectors',
    'Coq.Setoids', 'Coq.Classes', 'Coq.Relations', 'Coq.Sorting',
    'Coq.PArith', 'Coq.Numbers', 'Coq.Structures',
}

# ── 定理名正则 ──
THEOREM_RE = re.compile(
    r'^\s*(Theorem|Lemma|Corollary|Fact|Remark|Proposition|Property|Example|'
    r'Definition|Fixpoint|CoFixpoint|Inductive|CoInductive|Record|Structure)\s+'
    r'(\w+)\s*',
    re.MULTILINE
)

AXIOM_RE = re.compile(r'^\s*(Axiom|Parameter|Hypothesis|Conjecture)\s+(\w+)', re.MULTILINE)


def scan_coq_file(v_path: str, rel_path: str) -> dict:
    """扫描单个 .v 文件, 提取定理和公理"""
    with open(v_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 提取模块路径
    module_match = re.search(r'Module\s+(\w+(?:\.\w+)*)', content)
    module_name = module_match.group(1) if module_match else rel_path.replace('/', '.').replace('\\', '.')

    # 检查是否包含经典公理引用
    has_classical = any(ax in content for ax in CLASSICAL_AXIOMS)

    # 提取定理
    theorems = []
    for m in THEOREM_RE.finditer(content):
        kind, name = m.groups()
        # 跳过内部符号
        if name.startswith('_') or name.startswith('internal_'):
            continue
        theorems.append({
            'name': name,
            'kind': kind,
            'module': module_name,
            'file': rel_path,
        })

    # 提取公理
    axioms = []
    for m in AXIOM_RE.finditer(content):
        axioms.append({
            'name': m.group(2),
            'module': module_name,
            'file': rel_path,
        })

    constructive = not has_classical and not any(ax['name'] in CLASSICAL_AXIOMS for ax in axioms)

    return {
        'file': rel_path,
        'module': module_name,
        'constructive': constructive,
        'has_classical_axiom': has_classical,
        'n_theorems': len(theorems),
        'n_axioms': len(axioms),
        'theorems': theorems,
        'axioms': axioms,
    }


def scan_proof_library(base_dir: str, lib_name: str, ext: str) -> list:
    """扫描整个证明库"""
    results = []
    base = Path(base_dir) / lib_name
    if not base.exists():
        print(f'  [SKIP] {lib_name}: directory not found')
        return results

    files = list(base.rglob(f'*{ext}'))
    print(f'  {lib_name}: {len(files)} {ext} files')

    for fpath in files:
        rel = str(fpath.relative_to(base)).replace('\\', '/')
        try:
            info = scan_coq_file(str(fpath), rel)
            results.append(info)
        except Exception as e:
            print(f'    [ERR] {rel}: {e}')

    return results


def build_index(output_path: str):
    """构建完整索引"""
    print('扫描证明库...')

    all_results = []
    all_results.extend(scan_proof_library('D:/proof_libraries', 'coq', '.v'))

    # 汇总统计
    total_theorems = 0
    constructive_theorems = 0
    total_files = len(all_results)
    constructive_files = sum(1 for r in all_results if r['constructive'])
    has_axiom_files = sum(1 for r in all_results if r['has_classical_axiom'])

    for r in all_results:
        for t in r['theorems']:
            total_theorems += 1
            if r['constructive']:
                constructive_theorems += 1

    # 构建索引 JSON
    index = {
        'version': 1,
        'source': 'D:/proof_libraries',
        'stats': {
            'total_files': total_files,
            'constructive_files': constructive_files,
            'classical_axiom_files': has_axiom_files,
            'total_theorems': total_theorems,
            'constructive_theorems': constructive_theorems,
        },
        'libraries': {
            'coq': {
                'files': [r for r in all_results if r['constructive']],
                'classical_files': [r for r in all_results if not r['constructive']],
            }
        },
        'domains': {
            'math': ['Coq.Arith', 'Coq.NArith', 'Coq.ZArith', 'Coq.QArith',
                     'Coq.Numbers', 'Coq.Lists', 'Coq.Vectors', 'Coq.Bool'],
            'logic': ['Coq.Init', 'Coq.Logic', 'Coq.Classes', 'Coq.Setoids',
                      'Coq.Relations'],
            'program': ['Coq.Strings', 'Coq.Sorting', 'Coq.Structures'],
        }
    }

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(index, f, ensure_ascii=False, indent=1)

    print(f'\n=== 索引统计 ===')
    print(f'  文件: {total_files} (构造性: {constructive_files}, 含经典公理: {has_axiom_files})')
    print(f'  定理: {total_theorems} (构造性: {constructive_theorems})')
    print(f'  输出: {output_path}')
    print(f'  大小: {os.path.getsize(output_path) / 1024:.0f} KB')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='构造性证明库索引器')
    parser.add_argument('--output', default='D:/nexus/binary/.proof_index.json')
    args = parser.parse_args()
    build_index(args.output)
