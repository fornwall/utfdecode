#!/usr/bin/env python3

import sys

print("#ifndef UNICODE_CODE_POINTS_HPP_INCLUDED")
print("#define UNICODE_CODE_POINTS_HPP_INCLUDED")
print("")
print(" // NOTE: File generated by massage_unicode_data.py - do not edit")
print("")
print("#include <map>")
print("#include <cstdint>")
print("#include <general_category_values.hpp>")
print("")
print("struct code_point {")
print("    uint32_t numeric_value;")
print("    char const* name;")
print("    general_category_value_t category;")
print("    uint8_t canonical_combining_class;")
print("    bool bidi_mirrored;")
print("    uint32_t simple_uppercase_mapping;")
print("    uint32_t simple_lowercase_mapping;")
print("    uint32_t simple_titlecase_mapping;")
print("};")
print("")
print("std::map<uint32_t, code_point> unicode_code_points;")
print("static code_point code_points_array[] = {")

category_abbreviation_map = {
    'Lu': 'Uppercase_Letter',
    'Ll': 'Lowercase_Letter',
    'Lt': 'Titlecase_Letter',
    'LC': 'Cased_Letter',
    'Lm': 'Modifier_Letter',
    'Lo': 'Other_Letter',
    'L': 'Letter',
    'Mn': 'Nonspacing_Mark',
    'Mc': 'Spacing_Mark',
    'Me': 'Enclosing_Mark',
    'M': 'Mark',
    'Nd': 'Decimal_Number',
    'Nl': 'Letter_Number',
    'No': 'Other_Number',
    'N': 'Number',
    'Pc': 'Connector_Punctuation',
    'Pd': 'Dash_Punctuation',
    'Ps': 'Open_Punctuation',
    'Pe': 'Close_Punctuation',
    'Pi': 'Initial_Punctuation',
    'Pf': 'Final_Punctuation',
    'Po': 'Other_Punctuation',
    'P': 'Punctuation',
    'Sm': 'Math_Symbol',
    'Sc': 'Currency_Symbol',
    'Sk': 'Modifier_Symbol',
    'So': 'Other_Symbol',
    'S': 'Symbol',
    'Zs': 'Space_Separator',
    'Zl': 'Line_Separator',
    'Zp': 'Paragraph_Separator',
    'Z': 'Separator',
    'Cc': 'Control',
    'Cf': 'Format',
    'Cs': 'Surrogate',
    'Co': 'Private_Use',
    'Cn': 'Unassigned',
    'C': 'Other'
}


for line in open("UnicodeData.txt"):
    parts = line.split(';')
    numeric_value = int(parts[0], 16)

    # http://www.unicode.org/reports/tr44/#General_Category_Values
    name = parts[1]
    if name == '<control>' and parts[10]:
        # Field 10 is "Old name as published in Unicode 1.0 or ISO 6429 names
        # for control functions", which is more helpful then "<control>":
        name = parts[10]
    general_category = parts[2]
    canonical_combining_class = parts[3]
    bidi_mirrored = 'true' if parts[9] == 'Y' else 'false'
    simple_uppercase_mapping = 0 if parts[12] == '' else int(parts[12], 16)
    simple_lowercase_mapping = 0 if parts[13] == '' else int(parts[13], 16)
    simple_titlecase_mapping = simple_uppercase_mapping if parts[14] == '\n' else int(parts[14], 16)

    print(' { ' + str(numeric_value) +
            ', "' + name + '"' +
            ', general_category_value_t::' + category_abbreviation_map[general_category] +
            ', ' + canonical_combining_class +
            ', ' + bidi_mirrored +
            ', ' + str(simple_uppercase_mapping) +
            ', ' + str(simple_lowercase_mapping) +
            ', ' + str(simple_titlecase_mapping) +
            ' },')

print("};")
print("")
print("void unicode_code_points_initialize() {")
print("    for (unsigned int i = 0; i < sizeof(code_points_array) / sizeof(code_points_array[0]); i++) {")
print("        unicode_code_points[code_points_array[i].numeric_value] = code_points_array[i];")
print("    }")
print("}")
print("")
print("#endif")

