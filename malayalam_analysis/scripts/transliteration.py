import unicodedata
import sys

# Define the transliteration mappings

vowels_exclude = ['അ', 'ആ', 'ഇ', 'ഈ', 'ഉ', 'ഊ', 'ഋ', 'എ', 'ഏ', 'ഐ', 'ഒ', 'ഓ', 'ഔ', 'ം']

chillu = {
    'ൺ': 'ṇ',
    'ൻ': 'n',
    'ർ': 'ṟ',
    'ൽ': 'l',
    'ൾ': 'ḷ',
    'ൿ': 'k',
}

two_part_vowel_signs = {  # aka circumgraphs
    ('െ', 'ാ'): 'ൊ',
    ('ാ', 'െ'): 'ൊ',

    ('േ', 'ാ'): 'ോ',
    ('ാ', 'േ'): 'ോ',

    ('െ', 'ൗ'): 'ൌ',
    ('ൗ', 'െ'): 'ൌ',

    ('െ', 'െ'): 'ൈ',
}

transliteration_mappings = {
    'ാ': 'ā',
    'ി': 'i',
    'ീ': 'ī',
    'ു': 'u',
    'ൂ': 'ū',
    'ൃ': 'r̥',
    'ൄ': 'r̥̄',
    'ൢ': 'l̥',
    'ൣ': 'l̥̄',
    'െ': 'e',
    'േ': 'ē',
    'ൈ': 'ai',
    'ൊ': 'o',
    'ോ': 'ō',
    'ൌ': 'au',  # 0D4C (archaic)
    'ൗ': 'au',  # 0D57

    'അ': 'a',
    'ആ': 'ā',
    'ഇ': 'i',
    'ഈ': 'ī',
    'ഉ': 'u',
    'ഊ': 'ū',
    'ഋ': 'r̥',
    'ൠ': 'r̥̄',
    'ഌ': 'l̥',
    'ൡ': 'l̥̄',
    'എ': 'e',
    'ഏ': 'ē',
    'ഐ': 'ai',
    'ഒ': 'o',
    'ഓ': 'ō',
    'ഔ': 'au',

    'ക': 'k',
    'ഖ': 'kh',
    'ഗ': 'g',
    'ഘ': 'gh',
    'ങ': 'ṅ',
    'ച': 'c',
    'ഛ': 'ch',
    'ജ': 'j',
    'ഝ': 'jh',
    'ഞ': 'ñ',
    'ട': 'ṭ',
    'ഠ': 'ṭh',
    'ഡ': 'ḍ',
    'ഢ': 'ḍh',
    'ണ': 'ṇ',
    'ത': 't',
    'ഥ': 'th',
    'ദ': 'd',
    'ധ': 'dh',
    'ന': 'n',
    'പ': 'p',
    'ഫ': 'ph',
    'ബ': 'b',
    'ഭ': 'bh',
    'മ': 'm',
    'റ': 'ṟ',
    'റ്റ': 'ṯ',
    'ഩ': 'ṉ',
    'ഴ': 'ḻ',
    'യ': 'y',
    'ര': 'r',
    'ല': 'l',
    'ള': 'ḷ',
    'വ': 'v',
    'ശ': 'ś',
    'ഷ': 'ṣ',
    'സ': 's',
    'ഹ': 'h',
    'ം': 'ṁ',

    '൦': '0',
    '൧': '1',
    '൨': '2',
    '൩': '3',
    '൪': '4',
    '൫': '5',
    '൬': '6',
    '൭': '7',
    '൮': '8',
    '൯': '9',
}

# Set of characters that mark the end of a word
punctuation_set = set(r"""!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~""")
whitespace_set = set(' \t\n\r\v\f')
word_end_markers = punctuation_set.union(whitespace_set)
# Zero-width non-joiner
zwnj = chr(8204)
word_end_markers.add(zwnj)

def is_diacritic(char):
    if char == '\u0D02':
        return False
    category = unicodedata.category(char)
    return category.startswith('M')


def transliterate(text):
    transliterated_text = ''
    i = 0
    while i < len(text):
        char = text[i]

        # Check for the ligatures "ന്റ" and "റ്റ"
        if (char == 'ന' or char == 'റ') and i + 2 < len(text) and text[i + 1:i + 3] == '്റ':
            if char == 'ന':
                transliterated_text += 'nṯ'
            else:
                transliterated_text += 'ṯṯ'

            next_char = text[i + 3] if i + 3 < len(text) else None
            if next_char and not is_diacritic(next_char) and next_char != '\u0D4D':
                transliterated_text += 'a'
            i += 3

        # Check for two-part dependent vowel signs
        elif i + 1 < len(text) and (char, text[i + 1]) in two_part_vowel_signs:
            transliterated_text += transliteration_mappings[two_part_vowel_signs[(char, text[i + 1])]]
            i += 2

        elif char in transliteration_mappings:
            transliterated_text += transliteration_mappings[char]
            # 'a' follows most letters, unless its one of the following
            # todo: put a comment here which explains whats going on here, because I can't even remember now
            if not is_diacritic(char) and char not in vowels_exclude and (
                    i + 1 == len(text) or not is_diacritic(text[i + 1]) and text[i + 1] != '\u0D4D'):
                transliterated_text += 'a'

            i += 1

        # Check for chillus characters
        elif char in chillu:
            transliterated_text += chillu[char]
            i += 1

        # Check for chandrakkala (virama)
        elif char == '\u0D4D':  # ്
            # saṁvr̥tōkāram
            if i == len(text) - 1 \
                    or text[i + 1] in word_end_markers \
                    or unicodedata.category(text[i + 1]).startswith('Z'):  # All unicode separators
                transliterated_text += 'ŭ'
            i += 1

        else:  # If we weren't able to map it to something, we just copy it
            transliterated_text += char
            i += 1

    return transliterated_text


if __name__ == '__main__':
    while True:
        text = input("Enter malayalam text to transliterate:\n")
        if text.strip() == '':
            break
        print(transliterate(text))

        if len(sys.argv) != 2:
            print("Usage: python transliteration.py <input_file>")
            sys.exit(1)

        input_file = sys.argv[1]
        output_file = input_file.rsplit('.', 1)[0] + '_transliterated.txt'

        with open(input_file, 'r', encoding='utf-8') as infile, \
             open(output_file, 'w', encoding='utf-8') as outfile:
            for line in infile:
                outfile.write(transliterate(line))
        print(f"Transliterated file written to {output_file}")