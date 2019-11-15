#!/usr/bin/env python3

import sys

print("#ifndef UNICODE_CODE_BLOCKS_HPP_INCLUDED")
print("#define UNICODE_CODE_BLOCKS_HPP_INCLUDED")
print("")
print(" // NOTE: File generated by massage_unicode_blocks.py - do not edit")
print("")
print("#include <cstdint>")
print("")
print("struct unicode_block_t {")
print("    uint32_t start;")
print("    uint32_t end;")
print("    char const* name;")
print("};")
print("")
print("static unicode_block_t unicode_blocks[] = {")

for line in open("Blocks.txt"):
    if line.startswith('#') or not ';' in line:
        continue
    parts = line.split(';')
    (start_value, end_value) = parts[0].split('..')
    #numeric_value = int(parts[0], 16)
    block_name = parts[1].strip()

    print(f'    {{ 0x{start_value}, 0x{end_value}, "{block_name}" }},')

print("};")
print("""char const* get_block_name(uint32_t codepoint) {
        for (unsigned int i = 0; i < sizeof(unicode_blocks) / sizeof(unicode_block_t); i++) {
                unicode_block_t const& b = unicode_blocks[i];
                if (b.start <= codepoint && b.end >= codepoint) return b.name;
        }
        return nullptr;
}""")
print("")
print("#endif")
