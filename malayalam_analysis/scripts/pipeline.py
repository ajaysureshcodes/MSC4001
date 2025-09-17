#!/usr/bin/env python3
"""
Enhanced Malayalam Analysis Pipeline with Controlled Transliteration
Uses your transliteration script to create parallel corpora
"""

import os
import re
import subprocess
import json
from pathlib import Path
import shutil

# Import your transliterator (assuming it's saved as transliteration.py)
try:
    from transliteration import transliterate
    TRANSLITERATOR_AVAILABLE = True
except ImportError:
    print("Warning: transliteration.py not found. Place it in the same directory.")
    TRANSLITERATOR_AVAILABLE = False

class EnhancedMalayalamPipeline:
    def __init__(self, tawa_path, data_dir="malayalam_analysis"):
        self.tawa_path = Path(tawa_path)
        self.data_dir = Path(data_dir)
        self.setup_directories()
        
        # Malayalam Unicode range
        self.malayalam_range = range(0x0D00, 0x0D80)
        
    def setup_directories(self):
        """Create enhanced directory structure"""
        dirs = [
            'raw_texts', 
            'processed/malayalam_native', 
            'processed/malayalam_transliterated',
            'processed/mixed_content',
            'processed/combined_corpora',
            'models', 
            'results/classification', 
            'results/compression_analysis',
            'results/segmentation', 
            'results/evaluation',
            'scripts'
        ]
        
        for dir_name in dirs:
            (self.data_dir / dir_name).mkdir(parents=True, exist_ok=True)
    
    def is_malayalam_text(self, text, threshold=0.5):
        """Check if text is predominantly Malayalam"""
        malayalam_chars = sum(1 for char in text if ord(char) in self.malayalam_range)
        total_chars = len([char for char in text if not char.isspace()])
        
        if total_chars == 0:
            return False
        return (malayalam_chars / total_chars) >= threshold
    
    def clean_malayalam_text(self, text):
        """Clean and normalize Malayalam text"""
        # Keep Malayalam characters, basic punctuation, and whitespace
        cleaned = re.sub(r'[^\u0D00-\u0D7F\s\.\,\?\!\:\;\-\(\)]', ' ', text)
        # Normalize whitespace
        cleaned = re.sub(r'\s+', ' ', cleaned.strip())
        return cleaned
    
    def process_files_with_transliteration(self, raw_text_dir):
        """Process files and create parallel Malayalam/transliterated corpora"""
        if not TRANSLITERATOR_AVAILABLE:
            print("Error: transliteration.py not available")
            return None
            
        raw_path = Path(raw_text_dir)
        if not raw_path.exists():
            print(f"Error: {raw_text_dir} not found")
            return None
        
        stats = {
            'total_files': 0,
            'malayalam_files_processed': 0,
            'transliterated_files_created': 0,
            'mixed_files': 0,
            'skipped_files': 0
        }
        
        print("Processing files with transliteration...")
        
        for txt_file in raw_path.glob("*.txt"):
            stats['total_files'] += 1
            
            try:
                with open(txt_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                if len(content.strip()) < 100:  # Skip very short files
                    stats['skipped_files'] += 1
                    continue
                
                # Check if file contains significant Malayalam content
                if self.is_malayalam_text(content, threshold=0.3):
                    # Clean Malayalam content
                    cleaned_malayalam = self.clean_malayalam_text(content)
                    
                    if len(cleaned_malayalam) > 50:
                        # Save cleaned Malayalam version
                        malayalam_file = self.data_dir / 'processed/malayalam_native' / f"{txt_file.stem}_malayalam.txt"
                        with open(malayalam_file, 'w', encoding='utf-8') as f:
                            f.write(cleaned_malayalam)
                        
                        # Create transliterated version
                        try:
                            transliterated = transliterate(cleaned_malayalam)
                            transliterated_file = self.data_dir / 'processed/malayalam_transliterated' / f"{txt_file.stem}_transliterated.txt"
                            with open(transliterated_file, 'w', encoding='utf-8') as f:
                                f.write(transliterated)
                            
                            stats['malayalam_files_processed'] += 1
                            stats['transliterated_files_created'] += 1
                            
                        except Exception as e:
                            print(f"Transliteration failed for {txt_file}: {e}")
                
                # Also check for mixed content
                malayalam_ratio = self.get_malayalam_ratio(content)
                latin_ratio = self.get_latin_ratio(content)
                
                if malayalam_ratio > 0.1 and latin_ratio > 0.1:
                    mixed_file = self.data_dir / 'processed/mixed_content' / f"{txt_file.stem}_mixed.txt"
                    with open(mixed_file, 'w', encoding='utf-8') as f:
                        f.write(content)
                    stats['mixed_files'] += 1
                
                if stats['total_files'] % 500 == 0:
                    print(f"Processed {stats['total_files']} files...")
                    
            except Exception as e:
                print(f"Error processing {txt_file}: {e}")
                continue
        
        # Create combined corpora
        self.create_combined_corpora()
        
        # Save statistics
        with open(self.data_dir / 'processing_stats.json', 'w') as f:
            json.dump(stats, f, indent=2)
        
        print("\nProcessing complete!")
        for key, value in stats.items():
            print(f"{key}: {value}")
        
        return stats
    
    def get_malayalam_ratio(self, text):
        """Get ratio of Malayalam characters"""
        malayalam_chars = sum(1 for char in text if ord(char) in self.malayalam_range)
        total_chars = len([char for char in text if not char.isspace()])
        return malayalam_chars / total_chars if total_chars > 0 else 0
    
    def get_latin_ratio(self, text):
        """Get ratio of Latin characters"""
        latin_chars = sum(1 for char in text if char.isascii() and char.isalpha())
        total_chars = len([char for char in text if not char.isspace()])
        return latin_chars / total_chars if total_chars > 0 else 0
    
    def create_combined_corpora(self):
        """Create large combined files for training"""
        # Combine Malayalam native files
        malayalam_files = list((self.data_dir / 'processed/malayalam_native').glob("*.txt"))
        if malayalam_files:
            malayalam_corpus = self.data_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
            self.combine_files(malayalam_files[:2000], malayalam_corpus)  # Limit for manageability
        
        # Combine transliterated files
        transliterated_files = list((self.data_dir / 'processed/malayalam_transliterated').glob("*.txt"))
        if transliterated_files:
            transliterated_corpus = self.data_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
            self.combine_files(transliterated_files[:2000], transliterated_corpus)
        
        print("Combined corpora created")
    
    def combine_files(self, file_list, output_file):
        """Combine multiple files into one large corpus"""
        with open(output_file, 'w', encoding='utf-8') as outfile:
            for file_path in file_list:
                try:
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        content = infile.read().strip()
                        if content:
                            outfile.write(content + '\n\n')
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")
    
    def train_tawa_models(self):
        """Train Tawa models on both Malayalam and transliterated corpora"""
        results = {}
        
        # Train native Malayalam model
        malayalam_corpus = self.data_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
        if malayalam_corpus.exists():
            results['malayalam_native'] = self.train_single_model(
                malayalam_corpus, 
                self.data_dir / 'models/malayalam_native.model',
                alphabet_size=65536,
                model_name="Malayalam Native"
            )
        
        # Train transliterated model
        transliterated_corpus = self.data_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
        if transliterated_corpus.exists():
            results['malayalam_transliterated'] = self.train_single_model(
                transliterated_corpus,
                self.data_dir / 'models/malayalam_transliterated.model', 
                alphabet_size=512,  # Smaller alphabet for transliterated text
                model_name="Malayalam Transliterated"
            )
        
        return results
    
    def train_single_model(self, corpus_file, model_file, alphabet_size, model_name):
        """Train a single Tawa model"""
        print(f"Training {model_name} model...")
        
        try:
            # Use the train program
            cmd = [
                str(self.tawa_path / 'apps/train/train'),
                '-a', str(alphabet_size),
                '-O', '5',  # Order 5
                '-e', 'D',  # PPMD escape method
                '-i', str(corpus_file),
                '-o', str(model_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                print(f"{model_name} model trained successfully")
                return {
                    'success': True,
                    'model_file': str(model_file),
                    'alphabet_size': alphabet_size,
                    'corpus_size': corpus_file.stat().st_size,
                    'stderr': result.stderr
                }
            else:
                print(f"{model_name} training failed: {result.stderr}")
                return {
                    'success': False,
                    'error': result.stderr
                }
                
        except subprocess.TimeoutExpired:
            print(f"{model_name} training timed out")
            return {'success': False, 'error': 'timeout'}
        except Exception as e:
            print(f"Error training {model_name}: {e}")
            return {'success': False, 'error': str(e)}
    
    def run_compression_experiments(self):
        """Run cross-compression experiments"""
        experiments = {}
        
        malayalam_corpus = self.data_dir / 'processed/combined_corpora/malayalam_native_corpus.txt'
        transliterated_corpus = self.data_dir / 'processed/combined_corpora/malayalam_transliterated_corpus.txt'
        
        malayalam_model = self.data_dir / 'models/malayalam_native.model'
        transliterated_model = self.data_dir / 'models/malayalam_transliterated.model'
        
        if not all(f.exists() for f in [malayalam_corpus, transliterated_corpus]):
            return {'error': 'Required corpus files not found'}
        
        # Experiment 1: Native Malayalam with both models
        experiments['native_malayalam'] = {
            'with_native_model': self.compress_with_model(malayalam_corpus, malayalam_model, 65536),
            'with_transliterated_model': self.compress_with_model(malayalam_corpus, transliterated_model, 512)
        }
        
        # Experiment 2: Transliterated Malayalam with both models
        experiments['transliterated_malayalam'] = {
            'with_native_model': self.compress_with_model(transliterated_corpus, malayalam_model, 65536),
            'with_transliterated_model': self.compress_with_model(transliterated_corpus, transliterated_model, 512)
        }
        
        return experiments
    
    def compress_with_model(self, text_file, model_file, alphabet_size):
        """Compress text using specific model"""
        try:
            output_file = self.data_dir / 'results/compression_analysis' / f"compressed_{text_file.stem}_{model_file.stem}.dat"
            
            cmd = [
                str(self.tawa_path / 'apps/encode/encode'),
                '-a', str(alphabet_size),
                '-m', str(model_file),
                '-i', str(text_file),
                '-o', str(output_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0 and output_file.exists():
                original_size = text_file.stat().st_size
                compressed_size = output_file.stat().st_size
                
                return {
                    'success': True,
                    'original_size': original_size,
                    'compressed_size': compressed_size,
                    'compression_ratio': original_size / compressed_size if compressed_size > 0 else 0,
                    'bits_per_char': (compressed_size * 8) / original_size if original_size > 0 else 0
                }
            else:
                return {'success': False, 'error': result.stderr}
                
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    def run_full_pipeline(self, raw_text_dir):
        """Run the complete enhanced pipeline"""
        print("Enhanced Malayalam Analysis Pipeline")
        print("=" * 50)
        
        # Phase 1: Process files with transliteration
        processing_stats = self.process_files_with_transliteration(raw_text_dir)
        
        if not processing_stats or processing_stats['malayalam_files_processed'] == 0:
            print("No Malayalam files processed. Check your input directory.")
            return None
        
        # Phase 2: Train models
        training_results = self.train_tawa_models()
        
        # Phase 3: Run compression experiments
        compression_results = self.run_compression_experiments()
        
        # Save comprehensive results
        final_results = {
            'processing_stats': processing_stats,
            'training_results': training_results,
            'compression_experiments': compression_results
        }
        
        with open(self.data_dir / 'pipeline_results.json', 'w') as f:
            json.dump(final_results, f, indent=2)
        
        print("\nPipeline complete! Check results in:", self.data_dir)
        return final_results

def main():
    # Configuration - update these paths
    TAWA_PATH = "/mnt/f/Research/malayalam_tawa_project/Tawa/Tawa-1.1"
    RAW_TEXT_DIR = "/mnt/f/Research/malayalam_tawa_project/malayalam_analysis/raw_texts"
    
    pipeline = EnhancedMalayalamPipeline(TAWA_PATH)
    results = pipeline.run_full_pipeline(RAW_TEXT_DIR)
    
    if results:
        print("\nKey findings will be in:")
        print("- processing_stats: File categorization results")
        print("- training_results: Model training success/failure")  
        print("- compression_experiments: Cross-compression analysis")

if __name__ == "__main__":
    main()