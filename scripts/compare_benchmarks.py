#!/usr/bin/env python3
"""
Compare two benchmark results and detect performance regressions.
"""

import json
import sys
import argparse
from typing import Dict


def load_results(filepath: str) -> Dict:
    """Load benchmark results from JSON file."""
    with open(filepath, 'r') as f:
        return json.load(f)


def compare_benchmarks(old_results: Dict, new_results: Dict, threshold: float = 0.05):
    """
    Compare two benchmark results.
    
    Args:
        old_results: Previous benchmark results
        new_results: Current benchmark results
        threshold: Regression threshold (default 5%)
    """
    print("=" * 80)
    print("Benchmark Comparison")
    print("=" * 80)
    print()
    
    # Compare overall performance
    old_nps = old_results.get("average_nps", 0)
    new_nps = new_results.get("average_nps", 0)
    
    if old_nps > 0:
        change_pct = ((new_nps - old_nps) / old_nps) * 100
        
        print(f"Overall Performance:")
        print(f"  Previous: {old_nps:,} nps")
        print(f"  Current:  {new_nps:,} nps")
        print(f"  Change:   {change_pct:+.2f}%")
        
        if change_pct < -threshold * 100:
            print(f"  ⚠️  REGRESSION DETECTED (>{threshold*100:.0f}% slower)")
        elif change_pct > threshold * 100:
            print(f"  ✓ IMPROVEMENT (>{threshold*100:.0f}% faster)")
        else:
            print(f"  ✓ Performance stable")
        print()
    
    # Compare individual positions
    print("Position-by-Position Comparison:")
    print("-" * 80)
    print(f"{'Position':<20} {'Old NPS':<15} {'New NPS':<15} {'Change':<10} {'Status'}")
    print("-" * 80)
    
    regressions = []
    improvements = []
    
    old_positions = old_results.get("positions", {})
    new_positions = new_results.get("positions", {})
    
    for pos_name in sorted(new_positions.keys()):
        if pos_name not in old_positions:
            continue
        
        old_pos_nps = old_positions[pos_name].get("nps", 0)
        new_pos_nps = new_positions[pos_name].get("nps", 0)
        
        if old_pos_nps > 0:
            change_pct = ((new_pos_nps - old_pos_nps) / old_pos_nps) * 100
            
            if change_pct < -threshold * 100:
                status = "⚠️  SLOWER"
                regressions.append((pos_name, change_pct))
            elif change_pct > threshold * 100:
                status = "✓ FASTER"
                improvements.append((pos_name, change_pct))
            else:
                status = "✓ STABLE"
            
            print(f"{pos_name:<20} {old_pos_nps:<15,} {new_pos_nps:<15,} "
                  f"{change_pct:>+7.2f}%  {status}")
    
    print("-" * 80)
    print()
    
    # Summary
    print("Summary:")
    print(f"  Regressions:  {len(regressions)}")
    print(f"  Improvements: {len(improvements)}")
    print()
    
    if regressions:
        print("⚠️  Performance Regressions Detected:")
        for pos_name, change_pct in sorted(regressions, key=lambda x: x[1]):
            print(f"  - {pos_name}: {change_pct:.2f}%")
        print()
    
    if improvements:
        print("✓ Performance Improvements:")
        for pos_name, change_pct in sorted(improvements, key=lambda x: x[1], reverse=True):
            print(f"  - {pos_name}: {change_pct:+.2f}%")
        print()
    
    # Compare perft if available
    if "perft" in old_results and "perft" in new_results:
        print("Perft Comparison:")
        print("-" * 80)
        
        old_perft = old_results["perft"]
        new_perft = new_results["perft"]
        
        for depth_key in sorted(new_perft.keys()):
            if depth_key not in old_perft:
                continue
            
            old_nps = old_perft[depth_key].get("nps", 0)
            new_nps = new_perft[depth_key].get("nps", 0)
            
            if old_nps > 0:
                change_pct = ((new_nps - old_nps) / old_nps) * 100
                print(f"  {depth_key}: {change_pct:+.2f}% "
                      f"({old_nps:,} -> {new_nps:,} nps)")
        print()
    
    print("=" * 80)
    
    # Exit with error if significant regression
    if old_nps > 0 and ((new_nps - old_nps) / old_nps) < -threshold:
        print("\n❌ Significant performance regression detected!")
        return 1
    
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Compare two benchmark results"
    )
    parser.add_argument(
        "old_results",
        help="Path to old benchmark results JSON"
    )
    parser.add_argument(
        "new_results",
        help="Path to new benchmark results JSON"
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.05,
        help="Regression threshold (default: 0.05 = 5%%)"
    )
    parser.add_argument(
        "--fail-on-regression",
        action="store_true",
        help="Exit with error code if regression detected"
    )
    
    args = parser.parse_args()
    
    try:
        old_results = load_results(args.old_results)
        new_results = load_results(args.new_results)
        
        result = compare_benchmarks(old_results, new_results, args.threshold)
        
        if args.fail_on_regression and result != 0:
            sys.exit(1)
        
        return 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
