#!/usr/bin/env python3
"""
Build Malayalam corpora (native + transliterated) from raw_texts folder.
"""

import re
import unicodedata
from pathlib import Path
import sys

# ---------- SETTINGS ----------
RAW_DIR = Path("/mnt/f/Research/malayalam_tawa_project/malayalam_analysis/raw_texts")
OUTPUT_DIR = Path("/mnt/f/Research/malayalam_tawa_project/malayalam_analysis/processed/combined_corpora")

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
NATIVE_OUT = OUTPUT_DIR / "malayalam_native_corpus.txt"
TRANSLIT_OUT = OUTPUT_DIR / "malayalam_transliterated_corpus.txt"

# ---------- CLEANING ----------
def clean_malayalam(text: str) -> str:
    """Keep only Malayalam (U+0D00–U+0D7F), basic punctuation, and spaces"""
    text = re.sub(r"[^\u0D00-\u0D7F\s.,!?;:]", " ", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text

# ---------- TRANSLITERATION ----------
# (reuse your mappings)

vowels_exclude = ['അ','ആ','ഇ','ഈ','ഉ','ഊ','ഋ','എ','ഏ','ഐ','ഒ','ഓ','ഔ','ം']

chillu = {'ൺ':'ṇ','ൻ':'n','ർ':'ṟ','ൽ':'l','ൾ':'ḷ','ൿ':'k'}

two_part_vowel_signs = {
    ('െ','ാ'):'ൊ',('ാ','െ'):'ൊ',
    ('േ','ാ'):'ോ',('ാ','േ'):'ോ',
    ('െ','ൗ'):'ൌ',('ൗ','െ'):'ൌ',
    ('െ','െ'):'ൈ',
}

transliteration_mappings = {
    'ാ':'ā','ി':'i','ീ':'ī','ു':'u','ൂ':'ū','ൃ':'r̥','ൄ':'r̥̄','ൢ':'l̥','ൣ':'l̥̄',
    'െ':'e','േ':'ē','ൈ':'ai','ൊ':'o','ോ':'ō','ൌ':'au','ൗ':'au',
    'അ':'a','ആ':'ā','ഇ':'i','ഈ':'ī','ഉ':'u','ഊ':'ū','ഋ':'r̥','ൠ':'r̥̄',
    'ഌ':'l̥','ൡ':'l̥̄','എ':'e','ഏ':'ē','ഐ':'ai','ഒ':'o','ഓ':'ō','ഔ':'au',
    'ക':'k','ഖ':'kh','ഗ':'g','ഘ':'gh','ങ':'ṅ','ച':'c','ഛ':'ch','ജ':'j','ഝ':'jh','ഞ':'ñ',
    'ട':'ṭ','ഠ':'ṭh','ഡ':'ḍ','ഢ':'ḍh','ണ':'ṇ','ത':'t','ഥ':'th','ദ':'d','ധ':'dh','ന':'n',
    'പ':'p','ഫ':'ph','ബ':'b','ഭ':'bh','മ':'m',
    'റ':'ṟ','റ്റ':'ṯ','ഩ':'ṉ','ഴ':'ḻ','യ':'y','ര':'r','ല':'l','ള':'ḷ','വ':'v',
    'ശ':'ś','ഷ':'ṣ','സ':'s','ഹ':'h','ം':'ṁ',
    '൦':'0','൧':'1','൨':'2','൩':'3','൪':'4','൫':'5','൬':'6','൭':'7','൮':'8','൯':'9',
}

punctuation_set = set(r"""!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~""")
whitespace_set = set(' \t\n\r\v\f')
word_end_markers = punctuation_set.union(whitespace_set)
zwnj = chr(8204)
word_end_markers.add(zwnj)

def is_diacritic(char):
    if char == '\u0D02':
        return False
    return unicodedata.category(char).startswith('M')

def transliterate(text: str) -> str:
    result = ''
    i = 0
    while i < len(text):
        char = text[i]

        if (char == 'ന' or char == 'റ') and i+2 < len(text) and text[i+1:i+3] == '്റ':
            result += 'nṯ' if char == 'ന' else 'ṯṯ'
            next_char = text[i+3] if i+3 < len(text) else None
            if next_char and not is_diacritic(next_char) and next_char != '\u0D4D':
                result += 'a'
            i += 3

        elif i+1 < len(text) and (char,text[i+1]) in two_part_vowel_signs:
            result += transliteration_mappings[two_part_vowel_signs[(char,text[i+1])]]
            i += 2

        elif char in transliteration_mappings:
            result += transliteration_mappings[char]
            if not is_diacritic(char) and char not in vowels_exclude and (
                i+1==len(text) or not is_diacritic(text[i+1]) and text[i+1] != '\u0D4D'
            ):
                result += 'a'
            i += 1

        elif char in chillu:
            result += chillu[char]; i += 1

        elif char == '\u0D4D':  # virama
            if i == len(text)-1 or text[i+1] in word_end_markers or unicodedata.category(text[i+1]).startswith('Z'):
                result += 'ŭ'
            i += 1

        else:
            result += char; i += 1

    return result

# ---------- PIPELINE ----------
def build_corpora():
    with open(NATIVE_OUT, "w", encoding="utf-8") as fout_native, \
         open(TRANSLIT_OUT, "w", encoding="utf-8") as fout_translit:

        for file in RAW_DIR.glob("*.txt"):
            try:
                text = Path(file).read_text(encoding="utf-8", errors="ignore")
                text = clean_malayalam(text)
                if not text:
                    continue

                fout_native.write(text + "\n")
                fout_translit.write(transliterate(text) + "\n")

            except Exception as e:
                print(f"Error processing {file}: {e}")

    print(f"✓ Native corpus saved to {NATIVE_OUT}")
    print(f"✓ Transliterated corpus saved to {TRANSLIT_OUT}")

# ---------- MAIN ----------
if __name__ == "__main__":
    build_corpora()