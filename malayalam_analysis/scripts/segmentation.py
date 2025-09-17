#!/usr/bin/env python3
"""
Simple Compression-Based Segmentation Analysis
Analyzes word boundary understanding through compression of different spacing patterns
"""

import subprocess
import json
import re
from pathlib import Path
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from collections import Counter

class SimpleSegmentationAnalyzer:
    def __init__(self, tawa_path, analysis_dir):
        self.tawa_path = Path(tawa_path)
        self.analysis_dir = Path(analysis_dir)
        self.models_dir = self.analysis_dir / 'models'
        self.processed_dir = self.analysis_dir / 'processed/combined_corpora'
        self.results_dir = self.analysis_dir / 'results'
        self.figures_dir = self.analysis_dir / 'figures'
        
        # Create directories
        self.results_dir.mkdir(parents=True, exist_ok=True)
        self.figures_dir.mkdir(parents=True, exist_ok=True)
        
        # Find encode program
        self.encode_program = self.find_encode_program()
    
    def find_encode_program(self):
        """Find the encode program"""
        possible_paths = [
            self.tawa_path / 'apps/encode/encode',
            self.tawa_path / 'encode',
        ]
        
        for path in possible_paths:
            if path.exists() and path.is_file():
                print(f"Found encode program: {path}")
                return path
        
        print("ERROR: encode program not found")
        return None
    
    def create_spacing_variants(self, text, max_chars=2000):
        """Create different spacing variants from text"""
        # Limit text size for manageable processing
        if len(text) > max_chars:
            text = text[:max_chars]
            # Try to end at a sentence boundary
            last_period = text.rfind('.')
            if last_period > max_chars * 0.8:
                text = text[:last_period + 1]
        
        variants = {}
        
        # 1. Original spacing (reference)
        variants['original'] = text
        
        # 2. No spaces (worst case for word boundary understanding)
        variants['no_spaces'] = re.sub(r'\s+', '', text)
        
        # 3. Single character spacing (over-segmentation)
        no_space_text = re.sub(r'\s+', '', text)
        variants['char_spacing'] = ' '.join(no_space_text)
        
        # 4. Random spacing (control for comparison)
        chars = list(re.sub(r'\s+', '', text))
        if len(chars) > 10:
            # Insert spaces at random positions (roughly 1 every 3-5 chars)
            spaced_chars = []
            for i, char in enumerate(chars):
                spaced_chars.append(char)
                if i > 0 and i % 4 == 0 and i < len(chars) - 1:
                    spaced_chars.append(' ')
            variants['random_spacing'] = ''.join(spaced_chars)
        
        return variants
    
    def compress_text(self, text, model_file, alphabet_size, variant_name):
        """Compress text with a model and return compression stats"""
        if not self.encode_program or not model_file.exists():
            return {'success': False, 'error': 'Missing encode program or model'}
        
        # Create temporary files
        input_file = self.results_dir / f"temp_{variant_name}_input.txt"
        output_file = self.results_dir / f"temp_{variant_name}_output.dat"
        
        try:
            # Write input text
            with open(input_file, 'w', encoding='utf-8') as f:
                f.write(text)
            
            # Run compression
            cmd = [
                str(self.encode_program),
                '-a', str(alphabet_size),
                '-m', str(model_file),
                '-i', str(input_file),
                '-o', str(output_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0 and output_file.exists():
                original_size = input_file.stat().st_size
                compressed_size = output_file.stat().st_size
                
                # Parse bits per character from stderr if available
                bpc = self.parse_bpc_from_stderr(result.stderr)
                if not bpc:
                    bpc = (compressed_size * 8) / original_size if original_size > 0 else 0
                
                return {
                    'success': True,
                    'original_size': original_size,
                    'compressed_size': compressed_size,
                    'compression_ratio': original_size / compressed_size if compressed_size > 0 else 0,
                    'bits_per_char': bpc,
                    'stderr_output': result.stderr
                }
            else:
                return {
                    'success': False,
                    'error': result.stderr
                }
        
        except Exception as e:
            return {'success': False, 'error': str(e)}
        
        finally:
            # Clean up temporary files
            for temp_file in [input_file, output_file]:
                if temp_file.exists():
                    temp_file.unlink()
    
    def parse_bpc_from_stderr(self, stderr):
        """Parse bits per character from encode program output"""
        import re
        
        # Look for patterns like "2.345 bpc"
        patterns = [
            r'(\d+\.\d+)\s*bpc',
            r'bytes\s+input\s+\d+\s+bytes\s+output\s+\d+\s+(\d+\.\d+)\s*bpc'
        ]
        
        for pattern in patterns:
            match = re.search(pattern, stderr)
            if match:
                return float(match.group(1))
        
        return None
    
    def analyze_corpus_segmentation_patterns(self, corpus_file, model_file, alphabet_size, corpus_type):
        """Analyze segmentation patterns for a corpus"""
        
        try:
            with open(corpus_file, 'r', encoding='utf-8') as f:
                text = f.read()
            
            print(f"Analyzing {corpus_type} corpus ({len(text)} characters)")
            
            # Create spacing variants
            variants = self.create_spacing_variants(text)
            
            results = {}
            
            # Test compression for each variant
            for variant_name, variant_text in variants.items():
                print(f"  Testing {variant_name} spacing...")
                
                result = self.compress_text(
                    variant_text, 
                    model_file, 
                    alphabet_size, 
                    f"{corpus_type}_{variant_name}"
                )
                
                if result['success']:
                    # Add text statistics
                    words = variant_text.split()
                    result.update({
                        'text_length': len(variant_text),
                        'word_count': len(words),
                        'avg_word_length': np.mean([len(w) for w in words]) if words else 0,
                        'spaces_count': variant_text.count(' ')
                    })
                
                results[variant_name] = result
            
            return results
            
        except Exception as e:
            print(f"Error analyzing {corpus_type}: {e}")
            return None
    
    def run_segmentation_analysis(self):
        """Run complete segmentation analysis"""
        
        print("=" * 60)
        print("COMPRESSION-BASED SEGMENTATION ANALYSIS")
        print("=" * 60)
        
        if not self.encode_program:
            print("Cannot run analysis - encode program not found")
            return None
        
        # Define corpora and models
        corpora = {
            'malayalam_native': {
                'corpus': self.processed_dir / 'malayalam_native_corpus.txt',
                'model': self.models_dir / 'malayalam_native.model',
                'alphabet_size': 65536
            },
            'malayalam_transliterated': {
                'corpus': self.processed_dir / 'malayalam_transliterated_corpus.txt',
                'model': self.models_dir / 'malayalam_transliterated.model',
                'alphabet_size': 512
            }
        }
        
        # Check prerequisites
        missing_files = []
        for corpus_type, config in corpora.items():
            if not config['corpus'].exists():
                missing_files.append(str(config['corpus']))
            if not config['model'].exists():
                missing_files.append(str(config['model']))
        
        if missing_files:
            print("Missing required files:")
            for file in missing_files:
                print(f"  {file}")
            return None
        
        all_results = {}
        
        # Analyze each corpus with its own model
        for corpus_type, config in corpora.items():
            print(f"\nAnalyzing {corpus_type}...")
            
            results = self.analyze_corpus_segmentation_patterns(
                config['corpus'],
                config['model'],
                config['alphabet_size'],
                corpus_type
            )
            
            if results:
                all_results[f"{corpus_type}_with_own_model"] = results
        
        # Cross model analysis (Malayalam text with transliterated model for example)
        if len(corpora) >= 2:
            corpus_keys = list(corpora.keys())
            
            # Malayalam corpus with transliterated model
            print(f"\nCross-analysis: {corpus_keys[0]} text with {corpus_keys[1]} model...")
            cross_results1 = self.analyze_corpus_segmentation_patterns(
                corpora[corpus_keys[0]]['corpus'],
                corpora[corpus_keys[1]]['model'],
                corpora[corpus_keys[1]]['alphabet_size'],
                f"{corpus_keys[0]}_cross"
            )
            if cross_results1:
                all_results[f"{corpus_keys[0]}_with_{corpus_keys[1]}_model"] = cross_results1
            
            # Transliterated corpus with Malayalam model
            print(f"\nCross-analysis: {corpus_keys[1]} text with {corpus_keys[0]} model...")
            cross_results2 = self.analyze_corpus_segmentation_patterns(
                corpora[corpus_keys[1]]['corpus'],
                corpora[corpus_keys[0]]['model'],
                corpora[corpus_keys[0]]['alphabet_size'],
                f"{corpus_keys[1]}_cross"
            )
            if cross_results2:
                all_results[f"{corpus_keys[1]}_with_{corpus_keys[0]}_model"] = cross_results2
        
        # Saving results
        results_file = self.results_dir / 'segmentation_analysis_results.json'
        with open(results_file, 'w', encoding='utf-8') as f:
            json.dump(all_results, f, indent=2, default=str)
        
        # Generate analysis and visualizations
        self.analyze_results(all_results)
        self.create_visualizations(all_results)
        self.generate_report(all_results)
        
        print(f"\nAnalysis complete! Results saved to:")
        print(f"- {results_file}")
        print(f"- Figures in: {self.figures_dir}")
        
        return all_results
    
    def analyze_results(self, results):
        """Analyze the compression results to understand segmentation preferences"""
        
        analysis = {
            'spacing_preferences': {},
            'model_effectiveness': {},
            'key_insights': []
        }
        
        for experiment, spacing_results in results.items():
            if not spacing_results:
                continue
            
            # Find which spacing pattern compresses best (lowest BPC = best compression)
            valid_results = {k: v for k, v in spacing_results.items() 
                           if v.get('success') and v.get('bits_per_char', 0) > 0}
            
            if valid_results:
                best_spacing = min(valid_results.keys(), 
                                 key=lambda k: valid_results[k]['bits_per_char'])
                worst_spacing = max(valid_results.keys(),
                                  key=lambda k: valid_results[k]['bits_per_char'])
                
                best_bpc = valid_results[best_spacing]['bits_per_char']
                worst_bpc = valid_results[worst_spacing]['bits_per_char']
                
                analysis['spacing_preferences'][experiment] = {
                    'best_spacing': best_spacing,
                    'worst_spacing': worst_spacing,
                    'best_bpc': best_bpc,
                    'worst_bpc': worst_bpc,
                    'compression_range': worst_bpc - best_bpc,
                    'all_results': valid_results
                }
                
                # Generate insights
                if best_spacing == 'original':
                    analysis['key_insights'].append(
                        f"{experiment}: Model prefers original spacing (good word boundary understanding)"
                    )
                elif best_spacing == 'no_spaces':
                    analysis['key_insights'].append(
                        f"{experiment}: Model prefers no spaces (poor word boundary understanding)"
                    )
        
        # Save detailed analysis
        with open(self.results_dir / 'segmentation_analysis.json', 'w') as f:
            json.dump(analysis, f, indent=2, default=str)
        
        return analysis
    
    def create_visualizations(self, results):
        """Create visualizations of segmentation analysis"""
        
        # Extract data for plotting
        plot_data = []
        
        for experiment, spacing_results in results.items():
            for spacing_type, result in spacing_results.items():
                if result.get('success'):
                    plot_data.append({
                        'experiment': experiment.replace('_', ' ').title(),
                        'spacing_type': spacing_type.replace('_', ' ').title(),
                        'bits_per_char': result.get('bits_per_char', 0),
                        'compression_ratio': result.get('compression_ratio', 0)
                    })
        
        if not plot_data:
            print("No valid data for visualization")
            return
        
        # Create comprehensive visualization
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Compression-Based Segmentation Analysis', fontsize=16, fontweight='bold')
        
        # Plot 1: Bits per character by experiment and spacing
        experiments = list(set([d['experiment'] for d in plot_data]))
        spacing_types = list(set([d['spacing_type'] for d in plot_data]))
        
        # Prepare data for grouped bar chart
        experiment_data = {}
        for exp in experiments:
            experiment_data[exp] = {}
            for spacing in spacing_types:
                matching = [d for d in plot_data if d['experiment'] == exp and d['spacing_type'] == spacing]
                experiment_data[exp][spacing] = matching[0]['bits_per_char'] if matching else 0
        
        x = np.arange(len(experiments))
        width = 0.8 / len(spacing_types)
        
        colors = plt.cm.Set3(np.linspace(0, 1, len(spacing_types)))
        
        for i, spacing in enumerate(spacing_types):
            values = [experiment_data[exp].get(spacing, 0) for exp in experiments]
            ax1.bar(x + i * width, values, width, label=spacing, color=colors[i], alpha=0.8)
        
        ax1.set_xlabel('Experiment')
        ax1.set_ylabel('Bits Per Character (lower = better)')
        ax1.set_title('Compression Efficiency by Spacing Pattern')
        ax1.set_xticks(x + width * (len(spacing_types) - 1) / 2)
        ax1.set_xticklabels(experiments, rotation=45, ha='right')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # Plot 2: Compression ratios
        for i, spacing in enumerate(spacing_types):
            values = [experiment_data[exp].get(spacing, 0) for exp in experiments]
            # Get compression ratios instead
            comp_ratios = []
            for exp in experiments:
                matching = [d for d in plot_data if d['experiment'] == exp and d['spacing_type'] == spacing]
                comp_ratios.append(matching[0]['compression_ratio'] if matching else 0)
            
            ax2.bar(x + i * width, comp_ratios, width, label=spacing, color=colors[i], alpha=0.8)
        
        ax2.set_xlabel('Experiment')
        ax2.set_ylabel('Compression Ratio (higher = better)')
        ax2.set_title('Compression Ratios by Spacing Pattern')
        ax2.set_xticks(x + width * (len(spacing_types) - 1) / 2)
        ax2.set_xticklabels(experiments, rotation=45, ha='right')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        # Plot 3: Heatmap of bits per character
        heatmap_data = np.zeros((len(experiments), len(spacing_types)))
        for i, exp in enumerate(experiments):
            for j, spacing in enumerate(spacing_types):
                heatmap_data[i, j] = experiment_data[exp].get(spacing, 0)
        
        im = ax3.imshow(heatmap_data, cmap='YlOrRd', aspect='auto')
        ax3.set_xticks(range(len(spacing_types)))
        ax3.set_yticks(range(len(experiments)))
        ax3.set_xticklabels(spacing_types, rotation=45, ha='right')
        ax3.set_yticklabels(experiments)
        ax3.set_title('Bits Per Character Heatmap\n(Yellow=Better, Red=Worse)')
        
        # Add text annotations
        for i in range(len(experiments)):
            for j in range(len(spacing_types)):
                text = ax3.text(j, i, f'{heatmap_data[i, j]:.2f}',
                               ha="center", va="center", color="black", fontsize=8)
        
        plt.colorbar(im, ax=ax3, label='Bits Per Character')
        
        # Plot 4: Summary comparison (original vs no_spaces)
        original_data = [d for d in plot_data if d['spacing_type'] == 'Original']
        no_spaces_data = [d for d in plot_data if d['spacing_type'] == 'No Spaces']
        
        if original_data and no_spaces_data:
            orig_experiments = [d['experiment'] for d in original_data]
            orig_bpc = [d['bits_per_char'] for d in original_data]
            
            no_space_experiments = [d['experiment'] for d in no_spaces_data]
            no_space_bpc = [d['bits_per_char'] for d in no_spaces_data]
            
            x_pos = np.arange(len(orig_experiments))
            ax4.bar(x_pos - 0.2, orig_bpc, 0.4, label='Original Spacing', alpha=0.8)
            ax4.bar(x_pos + 0.2, no_space_bpc, 0.4, label='No Spaces', alpha=0.8)
            
            ax4.set_xlabel('Experiment')
            ax4.set_ylabel('Bits Per Character')
            ax4.set_title('Original vs No Spaces Comparison')
            ax4.set_xticks(x_pos)
            ax4.set_xticklabels(orig_experiments, rotation=45, ha='right')
            ax4.legend()
            ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.figures_dir / 'segmentation_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Visualization saved: {self.figures_dir / 'segmentation_analysis.png'}")
    
    def generate_report(self, results):
        """Generate a comprehensive text report"""
        
        report_lines = []
        report_lines.append("=" * 60)
        report_lines.append("COMPRESSION-BASED SEGMENTATION ANALYSIS REPORT")
        report_lines.append("=" * 60)
        report_lines.append("")
        
        # Summary statistics
        total_experiments = len(results)
        successful_experiments = sum(1 for r in results.values() if r and any(v.get('success') for v in r.values()))
        
        report_lines.append(f"Total experiments: {total_experiments}")
        report_lines.append(f"Successful experiments: {successful_experiments}")
        report_lines.append("")
        
        # Analysis for each experiment
        for experiment, spacing_results in results.items():
            if not spacing_results:
                continue
            
            report_lines.append(f"\n{experiment.replace('_', ' ').upper()}:")
            report_lines.append("-" * 40)
            
            # Find best and worst spacing patterns
            valid_results = {k: v for k, v in spacing_results.items() 
                           if v.get('success') and v.get('bits_per_char', 0) > 0}
            
            if valid_results:
                best_spacing = min(valid_results.keys(), 
                                 key=lambda k: valid_results[k]['bits_per_char'])
                worst_spacing = max(valid_results.keys(),
                                  key=lambda k: valid_results[k]['bits_per_char'])
                
                report_lines.append(f"Best spacing pattern: {best_spacing}")
                report_lines.append(f"  Bits per char: {valid_results[best_spacing]['bits_per_char']:.3f}")
                report_lines.append(f"  Compression ratio: {valid_results[best_spacing]['compression_ratio']:.2f}x")
                
                report_lines.append(f"Worst spacing pattern: {worst_spacing}")
                report_lines.append(f"  Bits per char: {valid_results[worst_spacing]['bits_per_char']:.3f}")
                report_lines.append(f"  Compression ratio: {valid_results[worst_spacing]['compression_ratio']:.2f}x")
                
                # Analysis
                if best_spacing == 'original':
                    report_lines.append("✓ Model shows good word boundary understanding")
                elif best_spacing == 'no_spaces':
                    report_lines.append("⚠ Model shows poor word boundary understanding")
                elif best_spacing == 'char_spacing':
                    report_lines.append("⚠ Model may be over-fitting to character patterns")
        
        # Key findings
        report_lines.append("\n\nKEY FINDINGS:")
        report_lines.append("-" * 40)
        
        # Compare native vs transliterated performance
        native_experiments = [k for k in results.keys() if 'native' in k.lower()]
        trans_experiments = [k for k in results.keys() if 'transliterated' in k.lower()]
        
        if native_experiments and trans_experiments:
            report_lines.append("• Both Malayalam native and transliterated models analyzed")
            report_lines.append("• Cross-model comparisons performed")
        
        report_lines.append("• Lower bits-per-character indicates better compression")
        report_lines.append("• Models that prefer original spacing show better word boundary understanding")
        
        # Save report
        report_file = self.results_dir / 'segmentation_analysis_report.txt'
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(report_lines))
        
        # Print to console
        print('\n'.join(report_lines[-20:]))  # Print last 20 lines
        print(f"\nFull report saved to: {report_file}")

def main():
    """Main execution"""
    
    # Configuration
    TAWA_PATH = "/mnt/f/Research/malayalam_tawa_project/Tawa/Tawa-1.1"
    ANALYSIS_DIR = "/mnt/f/Research/malayalam_tawa_project/malayalam_analysis"
    
    analyzer = SimpleSegmentationAnalyzer(TAWA_PATH, ANALYSIS_DIR)
    results = analyzer.run_segmentation_analysis()
    
    if results:
        print("\nSegmentation analysis complete!")
        print("This analysis shows how different spacing patterns affect compression,")
        print("indicating each model's understanding of word boundaries.")

if __name__ == "__main__":
    main()