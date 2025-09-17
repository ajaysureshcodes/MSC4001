#!/usr/bin/env python3
"""
Evaluation Framework for Malayalam vs Transliterated Malayalam Detection
"""

import os
import subprocess
from pathlib import Path
import json
import random
from collections import defaultdict

class MalayalamEvaluator:
    def __init__(self, tawa_path, analysis_dir):
        self.tawa_path = Path(tawa_path)
        self.analysis_dir = Path(analysis_dir)
        self.results_dir = self.analysis_dir / 'results/evaluation'
        
    def create_test_sets(self, test_ratio=0.2):
        """Create test sets from processed data"""
        
        # Get Malayalam files
        malayalam_files = self.analysis_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
        transliterated_files = self.analysis_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
        print(f"Malayalam files: {malayalam_files}")
        print(f"Transliterated files: {transliterated_files}")
        if not malayalam_files.exists() or not transliterated_files.exists():
            raise FileNotFoundError("Required corpus files not found.") 
        # Read lines from files
        with open(malayalam_files, 'r', encoding='utf-8') as f:
            malayalam_lines = f.readlines()

        with open(transliterated_files, 'r', encoding='utf-8') as f:
            transliterated_lines = f.readlines()
        # Create test sets
        random.shuffle(malayalam_lines)
        random.shuffle(transliterated_lines)

        # Split into train/test
        malayalam_test_count = int(len(malayalam_lines) * test_ratio)
        transliterated_test_count = int(len(transliterated_lines) * test_ratio)

        malayalam_test = malayalam_lines[:malayalam_test_count]
        malayalam_train = malayalam_lines[malayalam_test_count:]

        transliterated_test = transliterated_lines[:transliterated_test_count]
        transliterated_train = transliterated_lines[transliterated_test_count:]

        # Save the splits
        output_dir = self.results_dir
        output_dir.mkdir(parents=True, exist_ok=True)

        splits = {
            'malayalam_test': malayalam_test,
            'malayalam_train': malayalam_train,
            'transliterated_test': transliterated_test,
            'transliterated_train': transliterated_train
        }

        for name, lines in splits.items():
            with open(output_dir / f"{name}_corpus.txt", 'w', encoding='utf-8') as f:
                f.writelines(lines)
        
        return splits
    
    def calculate_compression_ratios(self):
        """Calculate compression ratios for different text types"""
        results = {}
        
        # Test Malayalam corpus
        malayalam_corpus = self.analysis_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
        if malayalam_corpus.exists():
            results['malayalam'] = self.compress_and_measure(malayalam_corpus, 65536)
        
        # Test transliterated corpus
        transliterated_corpus = self.analysis_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
        if transliterated_corpus.exists():
            results['transliterated'] = self.compress_and_measure(transliterated_corpus, 256)
        
        return results
    
    def compress_and_measure(self, corpus_file, alphabet_size):
        """Compress a corpus and measure compression ratio"""
        try:
            # Get original size
            original_size = corpus_file.stat().st_size
            
            # Compress using Tawa encode
            compressed_file = self.results_dir / f"{corpus_file.stem}_compressed.dat"
            cmd = [
                str(self.tawa_path / 'apps/encode/encode'),
                '-a', str(alphabet_size),
                '-O', '5',
                '-i', str(corpus_file),
                '-o', str(compressed_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0 and compressed_file.exists():
                compressed_size = compressed_file.stat().st_size
                compression_ratio = original_size / compressed_size if compressed_size > 0 else 0
                
                return {
                    'original_size': original_size,
                    'compressed_size': compressed_size,
                    'compression_ratio': compression_ratio,
                    'bits_per_char': (compressed_size * 8) / original_size if original_size > 0 else 0,
                    'success': True
                }
            else:
                return {'success': False, 'error': result.stderr}
                
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    def cross_compression_test(self):
        """Test how well each model compresses the other language"""
        results = {}
        
        malayalam_corpus = self.analysis_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
        transliterated_corpus = self.analysis_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
        malayalam_model = self.analysis_dir / 'models/malayalam_native.model'
        transliterated_model = self.analysis_dir / 'models/malayalam_transliterated.model'
        
        if not all([f.exists() for f in [malayalam_corpus, transliterated_corpus]]):
            return {'error': 'Required corpus files not found'}
        
        # Test Malayalam text with both models
        results['malayalam_text'] = {
            'with_malayalam_model': self.compress_with_model(malayalam_corpus, malayalam_model, 65536),
            'with_transliterated_model': self.compress_with_model(malayalam_corpus, transliterated_model, 256)
        }
        
        # Test transliterated text with both models  
        results['transliterated_text'] = {
            'with_malayalam_model': self.compress_with_model(transliterated_corpus, malayalam_model, 65536),
            'with_transliterated_model': self.compress_with_model(transliterated_corpus, transliterated_model, 256)
        }
        
        return results
    
    def compress_with_model(self, text_file, model_file, alphabet_size):
        """Compress text using a specific model"""
        try:
            output_file = self.results_dir / f"cross_test_{text_file.stem}_{model_file.stem}.dat"
            
            cmd = [
                str(self.tawa_path / 'apps/encode/encode'),
                '-a', str(alphabet_size),
                '-m', str(model_file),
                '-i', str(text_file),
                '-o', str(output_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0 and output_file.exists():
                original_size = text_file.stat().st_size
                compressed_size = output_file.stat().st_size
                
                return {
                    'compression_ratio': original_size / compressed_size if compressed_size > 0 else 0,
                    'bits_per_char': (compressed_size * 8) / original_size if original_size > 0 else 0,
                    'success': True
                }
            else:
                return {'success': False, 'error': result.stderr}
                
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    def test_segmentation_patterns(self):
        """Test word segmentation on both types of text"""
        results = {}
        
        # Create sample texts without spaces for segmentation testing
        malayalam_sample = self.create_segmentation_sample('malayalam')
        transliterated_sample = self.create_segmentation_sample('transliterated')
        
        if malayalam_sample:
            results['malayalam_segmentation'] = self.test_segmentation(malayalam_sample, 65536)
        
        if transliterated_sample:
            results['transliterated_segmentation'] = self.test_segmentation(transliterated_sample, 256)
        
        return results
    
    def create_segmentation_sample(self, text_type):
        """Create sample text for segmentation testing"""
        if text_type == 'malayalam':
            corpus_file = self.analysis_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
            alphabet_size = 65536
        else:
            corpus_file = self.analysis_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
            alphabet_size = 256
        
        if not corpus_file.exists():
            return None
            
        try:
            with open(corpus_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Take a sample and remove spaces
            sample = content[:1000]
            no_spaces = sample.replace(' ', '')
            
            sample_file = self.results_dir / f"{text_type}_segmentation_sample.txt"
            with open(sample_file, 'w', encoding='utf-8') as f:
                f.write(no_spaces)
            
            return sample_file
            
        except Exception as e:
            print(f"Error creating segmentation sample: {e}")
            return None
    
    def test_segmentation(self, sample_file, alphabet_size):
        """Test segmentation using Tawa"""
        try:
            output_file = self.results_dir / f"segmented_{sample_file.stem}.txt"
            
            # Note: This assumes you have a model trained for segmentation
            # You might need to adjust the model path
            cmd = [
                str(self.tawa_path / 'apps/transform/segment'),
                '-a', str(alphabet_size),
                '-V',  # Use Viterbi algorithm
                '-i', str(sample_file),
                '-o', str(output_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                return {
                    'success': True,
                    'output_file': str(output_file),
                    'command_output': result.stderr
                }
            else:
                return {
                    'success': False,
                    'error': result.stderr
                }
                
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    def run_evaluation(self):
        """Run complete evaluation suite"""
        print("Running Malayalam vs Transliterated Malayalam Evaluation")
        print("=" * 60)
        
        evaluation_results = {}
        
        # Test 1: Create test/train splits
        print("1. Creating test sets...")
        test_sets = self.create_test_sets()
        evaluation_results['test_sets'] = {k: len(v) for k, v in test_sets.items()}
        
        # Test 2: Basic compression ratios
        print("2. Calculating compression ratios...")
        compression_results = self.calculate_compression_ratios()
        evaluation_results['compression_ratios'] = compression_results
        
        # Test 3: Cross-compression testing
        print("3. Running cross-compression tests...")
        cross_results = self.cross_compression_test()
        evaluation_results['cross_compression'] = cross_results
        
        # Test 4: Segmentation pattern analysis
        print("4. Testing segmentation patterns...")
        segmentation_results = self.test_segmentation_patterns()
        evaluation_results['segmentation'] = segmentation_results
        
        # Save comprehensive results
        results_file = self.results_dir / 'evaluation_results.json'
        with open(results_file, 'w', encoding='utf-8') as f:
            json.dump(evaluation_results, f, indent=2, ensure_ascii=False)
        
        # Print summary
        self.print_evaluation_summary(evaluation_results)
        
        return evaluation_results
    
    def print_evaluation_summary(self, results):
        """Print a summary of evaluation results"""
        print("\nEVALUATION SUMMARY")
        print("=" * 40)
        
        # Compression ratios
        if 'compression_ratios' in results:
            print("\nCompression Ratios:")
            for text_type, data in results['compression_ratios'].items():
                if data.get('success'):
                    print(f"  {text_type}: {data['compression_ratio']:.2f}x ({data['bits_per_char']:.2f} bpc)")
                else:
                    print(f"  {text_type}: Failed - {data.get('error', 'Unknown error')}")
        
        # Cross-compression results
        if 'cross_compression' in results:
            print("\nCross-Compression Analysis:")
            cc = results['cross_compression']
            if 'malayalam_text' in cc:
                mal_results = cc['malayalam_text']
                print("  Malayalam text:")
                if mal_results.get('with_malayalam_model', {}).get('success'):
                    print(f"    With Malayalam model: {mal_results['with_malayalam_model']['compression_ratio']:.2f}x")
                if mal_results.get('with_transliterated_model', {}).get('success'):
                    print(f"    With Transliterated model: {mal_results['with_transliterated_model']['compression_ratio']:.2f}x")
            
            if 'transliterated_text' in cc:
                trans_results = cc['transliterated_text']
                print("  Transliterated text:")
                if trans_results.get('with_malayalam_model', {}).get('success'):
                    print(f"    With Malayalam model: {trans_results['with_malayalam_model']['compression_ratio']:.2f}x")
                if trans_results.get('with_transliterated_model', {}).get('success'):
                    print(f"    With Transliterated model: {trans_results['with_transliterated_model']['compression_ratio']:.2f}x")
        
        print(f"\nDetailed results saved in: {self.results_dir / 'evaluation_results.json'}")

# Quick setup and run script
def main():
    TAWA_PATH = "/mnt/f/Research/malayalam_tawa_project/Tawa/Tawa-1.1"
    ANALYSIS_DIR = "/mnt/f/Research/malayalam_tawa_project/malayalam_analysis"
    
    evaluator = MalayalamEvaluator(TAWA_PATH, ANALYSIS_DIR)
    results = evaluator.run_evaluation()
    
    return results

if __name__ == "__main__":
    main()