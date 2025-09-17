#!/usr/bin/env python3
"""
Analysis of Malayalam Segmentation Results
Processes the JSON results and create analysis
"""

import json
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path

results_data = {
  "malayalam_native_with_own_model": {
    "original": {
      "success": True,
      "bits_per_char": 1.31,
      "compression_ratio": 6.11,
      "word_count": 212,
      "avg_word_length": 8.38
    },
    "no_spaces": {
      "success": True,
      "bits_per_char": 1.525,
      "compression_ratio": 5.25,
      "word_count": 1,
      "avg_word_length": 1776.0
    },
    "char_spacing": {
      "success": True,
      "bits_per_char": 3.953,
      "compression_ratio": 2.02,
      "word_count": 1776,
      "avg_word_length": 1.0
    },
    "random_spacing": {
      "success": True,
      "bits_per_char": 2.271,
      "compression_ratio": 3.52,
      "word_count": 444,
      "avg_word_length": 4.0
    }
  },
  "malayalam_transliterated_with_own_model": {
    "original": {
      "success": True,
      "bits_per_char": 1.477,
      "compression_ratio": 5.42,
      "word_count": 187,
      "avg_word_length": 9.02
    },
    "no_spaces": {
      "success": True,
      "bits_per_char": 2.36,
      "compression_ratio": 3.39,
      "word_count": 1,
      "avg_word_length": 1686.0
    },
    "char_spacing": {
      "success": True,
      "bits_per_char": 5.733,
      "compression_ratio": 1.40,
      "word_count": 1686,
      "avg_word_length": 1.0
    },
    "random_spacing": {
      "success": True,
      "bits_per_char": 4.327,
      "compression_ratio": 1.85,
      "word_count": 422,
      "avg_word_length": 4.0
    }
  },
  "malayalam_native_with_malayalam_transliterated_model": {
    "original": {
      "success": True,
      "bits_per_char": 7.62,
      "compression_ratio": 1.05,
      "word_count": 212,
      "avg_word_length": 8.38
    },
    "no_spaces": {
      "success": True,
      "bits_per_char": 7.649,
      "compression_ratio": 1.05,
      "word_count": 1,
      "avg_word_length": 1776.0
    },
    "char_spacing": {
      "success": True,
      "bits_per_char": 7.439,
      "compression_ratio": 1.08,
      "word_count": 1776,
      "avg_word_length": 1.0
    },
    "random_spacing": {
      "success": True,
      "bits_per_char": 7.593,
      "compression_ratio": 1.05,
      "word_count": 444,
      "avg_word_length": 4.0
    }
  },
  "malayalam_transliterated_with_malayalam_native_model": {
    "original": {
      "success": True,
      "bits_per_char": 15.681,
      "compression_ratio": 0.51,
      "word_count": 187,
      "avg_word_length": 9.02
    },
    "no_spaces": {
      "success": True,
      "bits_per_char": 16.394,
      "compression_ratio": 0.49,
      "word_count": 1,
      "avg_word_length": 1686.0
    },
    "char_spacing": {
      "success": True,
      "bits_per_char": 12.704,
      "compression_ratio": 0.63,
      "word_count": 1686,
      "avg_word_length": 1.0
    },
    "random_spacing": {
      "success": True,
      "bits_per_char": 15.059,
      "compression_ratio": 0.53,
      "word_count": 422,
      "avg_word_length": 4.0
    }
  }
}

def analyze_segmentation_patterns():
    """Analyze the key patterns in the segmentation results"""
    
    analysis = {
        'key_findings': [],
        'word_boundary_understanding': {},
        'cross_model_performance': {},
        'compression_effectiveness': {}
    }
    
    for experiment, results in results_data.items():
        original_bpc = results['original']['bits_per_char']
        no_spaces_bpc = results['no_spaces']['bits_per_char']
        
        # Lower BPC = better compression
        prefers_original = original_bpc < no_spaces_bpc
        difference = no_spaces_bpc - original_bpc
        
        analysis['word_boundary_understanding'][experiment] = {
            'prefers_original_spacing': prefers_original,
            'bpc_difference': difference,
            'understanding_quality': 'good' if prefers_original and difference > 0.1 else 'poor'
        }
    
    native_with_native = results_data['malayalam_native_with_own_model']['original']['bits_per_char']
    native_with_trans = results_data['malayalam_native_with_malayalam_transliterated_model']['original']['bits_per_char']
    
    trans_with_trans = results_data['malayalam_transliterated_with_own_model']['original']['bits_per_char']
    trans_with_native = results_data['malayalam_transliterated_with_malayalam_native_model']['original']['bits_per_char']
    
    analysis['cross_model_performance'] = {
        'native_text_prefers_native_model': native_with_native < native_with_trans,
        'transliterated_text_prefers_trans_model': trans_with_trans < trans_with_native,
        'models_distinguish_correctly': (native_with_native < native_with_trans) and (trans_with_trans < trans_with_native)
    }
    
    if analysis['cross_model_performance']['models_distinguish_correctly']:
        analysis['key_findings'].append("✓ Models successfully distinguish between Malayalam script types")
    
    native_understanding = analysis['word_boundary_understanding']['malayalam_native_with_own_model']
    trans_understanding = analysis['word_boundary_understanding']['malayalam_transliterated_with_own_model']
    
    if native_understanding['understanding_quality'] == 'good':
        analysis['key_findings'].append("✓ Malayalam native model shows good word boundary understanding")
    
    if trans_understanding['understanding_quality'] == 'good':
        analysis['key_findings'].append("✓ Transliterated model shows good word boundary understanding")
    
    if native_with_native < trans_with_trans:
        analysis['key_findings'].append("• Malayalam native script compresses better than transliterated")
    else:
        analysis['key_findings'].append("• Transliterated text compresses better than Malayalam native")
    
    return analysis

def create_comprehensive_visualizations():
    """Create comprehensive visualizations of the results"""
    
    experiments = list(results_data.keys())
    spacing_types = ['original', 'no_spaces', 'char_spacing', 'random_spacing']
    
    bpc_data = np.zeros((len(experiments), len(spacing_types)))
    compression_data = np.zeros((len(experiments), len(spacing_types)))
    
    exp_labels = []
    for i, exp in enumerate(experiments):
        if 'native_with_own' in exp:
            exp_labels.append('Malayalam\nNative Model')
        elif 'transliterated_with_own' in exp:
            exp_labels.append('Transliterated\nOwn Model')
        elif 'native_with_malayalam_transliterated' in exp:
            exp_labels.append('Malayalam Native\nTrans Model')
        else:
            exp_labels.append('Transliterated\nNative Model')
        
        for j, spacing in enumerate(spacing_types):
            bpc_data[i, j] = results_data[exp][spacing]['bits_per_char']
            compression_data[i, j] = results_data[exp][spacing]['compression_ratio']
    
    fig = plt.figure(figsize=(20, 16))
    gs = fig.add_gridspec(3, 3, hspace=0.3, wspace=0.3)
    
    fig.suptitle('Malayalam Segmentation Analysis Results\nCompression-Based Word Boundary Understanding', 
                 fontsize=16, fontweight='bold')
    
    ax1 = fig.add_subplot(gs[0, :2])
    im1 = ax1.imshow(bpc_data, cmap='RdYlGn_r', aspect='auto')
    ax1.set_xticks(range(len(spacing_types)))
    ax1.set_yticks(range(len(experiments)))
    ax1.set_xticklabels(['Original', 'No Spaces', 'Char Spacing', 'Random'])
    ax1.set_yticklabels(exp_labels)
    ax1.set_title('Bits Per Character (Lower = Better Compression)')
    
    for i in range(len(experiments)):
        for j in range(len(spacing_types)):
            text = ax1.text(j, i, f'{bpc_data[i, j]:.2f}',
                           ha="center", va="center", color="black", fontweight='bold')
    
    plt.colorbar(im1, ax=ax1, label='Bits Per Character')
    
    # 2. Word Boundary Understanding Comparison
    ax2 = fig.add_subplot(gs[0, 2])
    
    # Compare original vs no_spaces for each experiment
    original_bpc = [results_data[exp]['original']['bits_per_char'] for exp in experiments]
    no_spaces_bpc = [results_data[exp]['no_spaces']['bits_per_char'] for exp in experiments]
    
    x = np.arange(len(experiments))
    width = 0.35
    
    bars1 = ax2.bar(x - width/2, original_bpc, width, label='Original Spacing', alpha=0.8)
    bars2 = ax2.bar(x + width/2, no_spaces_bpc, width, label='No Spaces', alpha=0.8)
    
    ax2.set_xlabel('Experiment')
    ax2.set_ylabel('Bits Per Character')
    ax2.set_title('Word Boundary Understanding\n(Lower Original = Better)')
    ax2.set_xticks(x)
    ax2.set_xticklabels(exp_labels, rotation=45, ha='right')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    ax3 = fig.add_subplot(gs[1, 0])
    
    native_scores = [
        results_data['malayalam_native_with_own_model']['original']['bits_per_char'],
        results_data['malayalam_native_with_malayalam_transliterated_model']['original']['bits_per_char']
    ]
    
    trans_scores = [
        results_data['malayalam_transliterated_with_own_model']['original']['bits_per_char'],
        results_data['malayalam_transliterated_with_malayalam_native_model']['original']['bits_per_char']
    ]
    
    x = [0, 1]
    ax3.plot(x, native_scores, 'o-', linewidth=3, markersize=10, label='Malayalam Native Text')
    ax3.plot(x, trans_scores, 's-', linewidth=3, markersize=10, label='Transliterated Text')
    
    ax3.set_xticks(x)
    ax3.set_xticklabels(['With Own Model', 'With Other Model'])
    ax3.set_ylabel('Bits Per Character')
    ax3.set_title('Cross-Model Performance')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    ax4 = fig.add_subplot(gs[1, 1])
    
    original_ratios = [results_data[exp]['original']['compression_ratio'] for exp in experiments]
    
    bars = ax4.bar(range(len(experiments)), original_ratios, alpha=0.8, color='skyblue')
    ax4.set_xlabel('Experiment')
    ax4.set_ylabel('Compression Ratio')
    ax4.set_title('Compression Ratios\n(Original Spacing)')
    ax4.set_xticks(range(len(experiments)))
    ax4.set_xticklabels(exp_labels, rotation=45, ha='right')
    ax4.grid(True, alpha=0.3)
    
    for i, bar in enumerate(bars):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}x', ha='center', va='bottom')
    
    # 5. Spacing Pattern Effectiveness
    ax5 = fig.add_subplot(gs[1, 2])
    
    avg_bpc_by_spacing = []
    for spacing in spacing_types:
        avg_bpc = np.mean([results_data[exp][spacing]['bits_per_char'] for exp in experiments])
        avg_bpc_by_spacing.append(avg_bpc)
    
    bars = ax5.bar(spacing_types, avg_bpc_by_spacing, alpha=0.8, color='lightcoral')
    ax5.set_xlabel('Spacing Pattern')
    ax5.set_ylabel('Average Bits Per Character')
    ax5.set_title('Spacing Pattern Effectiveness\n(Across All Models)')
    ax5.set_xticklabels(['Original', 'No Spaces', 'Character', 'Random'], rotation=45, ha='right')
    ax5.grid(True, alpha=0.3)
    
    ax6 = fig.add_subplot(gs[2, :])
    
    word_data = []
    labels = []
    
    for exp in experiments:
        for spacing in ['original', 'no_spaces', 'char_spacing']:
            if spacing in results_data[exp]:
                word_data.append(results_data[exp][spacing]['avg_word_length'])
                labels.append(f"{exp.split('_')[1][:4]}\n{spacing[:4]}")
    
    bars = ax6.bar(range(len(word_data)), word_data, alpha=0.8)
    ax6.set_xlabel('Experiment + Spacing Type')
    ax6.set_ylabel('Average Word Length')
    ax6.set_title('Average Word Length by Experiment and Spacing Pattern')
    ax6.set_xticks(range(len(labels)))
    ax6.set_xticklabels(labels, rotation=45, ha='right')
    ax6.grid(True, alpha=0.3)
    ax6.set_yscale('log')  # Log scale due to large differences
    
    plt.tight_layout()
    
    output_dir = Path("malayalam_analysis/figures")
    output_dir.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_dir / 'segmentation_results_analysis.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    return str(output_dir / 'segmentation_results_analysis.png')

def generate_detailed_report():
    """Generate detailed analysis report"""
    
    analysis = analyze_segmentation_patterns()
    
    report = []
    report.append("=" * 70)
    report.append("MALAYALAM SEGMENTATION ANALYSIS - DETAILED RESULTS")
    report.append("=" * 70)
    report.append("")
    
    report.append("EXECUTIVE SUMMARY")
    report.append("-" * 50)
    for finding in analysis['key_findings']:
        report.append(f"  {finding}")
    report.append("")
    
    report.append("DETAILED ANALYSIS")
    report.append("-" * 50)
    report.append("")
    
    report.append("1. WORD BOUNDARY UNDERSTANDING:")
    report.append("   (Models that compress 'original' better than 'no spaces' understand word boundaries)")
    report.append("")
    
    for exp, data in analysis['word_boundary_understanding'].items():
        exp_name = exp.replace('_', ' ').title()
        status = "✓ GOOD" if data['understanding_quality'] == 'good' else "✗ POOR"
        report.append(f"   {exp_name}:")
        report.append(f"     Status: {status}")
        report.append(f"     BPC Difference: {data['bpc_difference']:.3f}")
        report.append("")
    
    report.append("2. CROSS-MODEL PERFORMANCE:")
    report.append("")
    
    cross_perf = analysis['cross_model_performance']
    
    report.append(f"   Malayalam native text prefers native model: {cross_perf['native_text_prefers_native_model']}")
    report.append(f"   Transliterated text prefers trans model: {cross_perf['transliterated_text_prefers_trans_model']}")
    report.append(f"   Models distinguish correctly: {cross_perf['models_distinguish_correctly']}")
    report.append("")
    
    report.append("3. COMPRESSION PERFORMANCE SUMMARY:")
    report.append("")
    
    # Best performing configurations
    best_compression = None
    best_bpc = float('inf')
    
    for exp, results in results_data.items():
        original_bpc = results['original']['bits_per_char']
        if original_bpc < best_bpc:
            best_bpc = original_bpc
            best_compression = exp
    
    report.append(f"   Best compression: {best_compression.replace('_', ' ')}")
    report.append(f"   Bits per character: {best_bpc}")
    report.append("")
    
    report.append("4. SPECIFIC METRICS:")
    report.append("")
    
    for exp, results in results_data.items():
        report.append(f"   {exp.replace('_', ' ').upper()}:")
        report.append(f"     Original spacing: {results['original']['bits_per_char']:.3f} BPC")
        report.append(f"     No spaces: {results['no_spaces']['bits_per_char']:.3f} BPC")
        report.append(f"     Compression ratio: {results['original']['compression_ratio']:.2f}x")
        report.append("")
    
    report.append("5. DISSERTATION INSIGHTS:")
    report.append("")
    report.append("   • Malayalam script and transliteration have distinct compression signatures")
    report.append("   • PPM models learn orthography-specific word boundary patterns")
    report.append("   • Cross-model testing reveals script-specific statistical structures")
    report.append("   • Compression-based analysis provides quantitative evidence of linguistic differences")
    report.append("")
    
    native_improvement = (results_data['malayalam_native_with_malayalam_transliterated_model']['original']['bits_per_char'] - 
                         results_data['malayalam_native_with_own_model']['original']['bits_per_char'])
    
    trans_improvement = (results_data['malayalam_transliterated_with_malayalam_native_model']['original']['bits_per_char'] - 
                        results_data['malayalam_transliterated_with_own_model']['original']['bits_per_char'])
    
    report.append("6. QUANTITATIVE EVIDENCE:")
    report.append("")
    report.append(f"   Native model advantage for Malayalam text: {native_improvement:.3f} BPC improvement")
    report.append(f"   Transliterated model advantage: {trans_improvement:.3f} BPC improvement")
    report.append(f"   Total distinguishability: {native_improvement + trans_improvement:.3f} BPC")
    
    report_text = '\n'.join(report)
    output_dir = Path("malayalam_analysis/results")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    with open(output_dir / 'segmentation_detailed_analysis.txt', 'w', encoding='utf-8') as f:
        f.write(report_text)
    
    return report_text, str(output_dir / 'segmentation_detailed_analysis.txt')

if __name__ == "__main__":
    print("Analyzing segmentation results...")
    
    analysis = analyze_segmentation_patterns()
    
    viz_path = create_comprehensive_visualizations()
    print(f"Visualization saved: {viz_path}")
  
    report_text, report_path = generate_detailed_report()
    print(f"Report saved: {report_path}")
    
    print("\nKEY FINDINGS:")
    for finding in analysis['key_findings']:
        print(f"  {finding}")