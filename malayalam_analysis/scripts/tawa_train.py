#!/usr/bin/env python3
"""
Custom Tawa Training Script for Malayalam
"""

import subprocess
import sys
from pathlib import Path

def train_malayalam_model(tawa_path, corpus_file, model_file, alphabet_size, title):
    """Train a single Malayalam model using available Tawa tools"""
    
    # Try method 1: train program
    train_cmd = [
        str(Path(tawa_path) / 'apps/train/train'),
        '-a', str(alphabet_size),
        '-O', '5',
        '-e', 'D',
        '-T', title,
        '-S',
        '-i', str(corpus_file),
        '-o', str(model_file)
    ]
    
    try:
        print(f"Training {title}...")
        result = subprocess.run(train_cmd, capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0:
            print(f"✓ Successfully trained {title}")
            return True
        else:
            print(f"Train program failed: {result.stderr}")
    except Exception as e:
        print(f"Train program not available: {e}")
    
    # Try method 2: encode program
    encode_cmd = [
        str(Path(tawa_path) / 'apps/encode/encode'),
        '-a', str(alphabet_size),
        '-O', '5',
        '-e', 'D',
        '-i', str(corpus_file),
        '-o', str(model_file)
    ]
    
    try:
        print(f"Training {title} using encode method...")
        result = subprocess.run(encode_cmd, capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0 and Path(model_file).exists():
            print(f"✓ Successfully trained {title} using encode")
            return True
        else:
            print(f"Encode method failed: {result.stderr}")
    except Exception as e:
        print(f"Encode method failed: {e}")
    
    return False

# Usage
if __name__ == "__main__":
    TAWA_PATH = "/mnt/f/Research/malayalam_tawa_project/Tawa/Tawa-1.1"
    ANALYSIS_DIR = "/mnt/f/Research/malayalam_tawa_project/malayalam_analysis"
    
    # Train Malayalam native model
    success1 = train_malayalam_model(
        TAWA_PATH,
        f"{ANALYSIS_DIR}/processed/combined_corpora/malayalam_native_corpus.txt",
        f"{ANALYSIS_DIR}/models/malayalam_native.model",
        65536,
        "Malayalam Native Script Model"
    )
    
    # Train transliterated model
    success2 = train_malayalam_model(
        TAWA_PATH,
        f"{ANALYSIS_DIR}/processed/combined_corpora/malayalam_transliterated_corpus.txt", 
        f"{ANALYSIS_DIR}/models/malayalam_transliterated.model",
        512,
        "Malayalam Transliterated Model"
    )
    
    if success1 and success2:
        print("\n✓ All models trained successfully!")
    else:
        print("\n✗ Some models failed to train")