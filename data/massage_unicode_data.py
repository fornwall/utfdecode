#!/usr/bin/env python3

import sys

print(" // NOTE: File generated by massage_unicode_data.py - do not edit")
print("")
print("#include <cstdint>")
print("#include <iomanip>")
print("#include <map>")
print("#include <string>")
print("#include <sstream>")
print("#include \"utfdecode.hpp\"")
print("")
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

array_index = 0
code_point_to_array_index = {}
first_numeric_value = -1

check_string = ""
check_string_metadata = ""

for line in open("UnicodeData.txt"):
    parts = line.split(';')
    numeric_value = int(parts[0], 16)
    name = parts[1]

    # D800;<Non Private Use High Surrogate, First>;Cs;0;L;;;;;N;;;;;
    # DB7F;<Non Private Use High Surrogate, Last>;Cs;0;L;;;;;N;;;;;
    # DB80;<Private Use High Surrogate, First>;Cs;0;L;;;;;N;;;;;
    # DBFF;<Private Use High Surrogate, Last>;Cs;0;L;;;;;N;;;;;
    # DC00;<Low Surrogate, First>;Cs;0;L;;;;;N;;;;;
    # DFFF;<Low Surrogate, Last>;Cs;0;L;;;;;N;;;;;
    # E000;<Private Use, First>;Co;0;L;;;;;N;;;;;
    # F8FF;<Private Use, Last>;Co;0;L;;;;;N;;;;;
    # and others...
    #if numeric_value in [0xD800, 0xDB7F, 0xDB80, 0xDBFF, 0xDC00, 0xDFFF, 0xE000, 0xF8FF]: continue
    if name.endswith(', First>'):
        first_numeric_value = numeric_value
        continue
    if name.endswith(', Last>'):
        name = name[1:-7].upper() # "<xxx, Last>" -> "XXX"
        last_numeric_value = numeric_value

        if check_string: check_string += " else "
        else: check_string = "    "
        check_string += "if (code_point >= " + hex(first_numeric_value) + " && code_point <= " + hex(last_numeric_value) + ") {\n"
        check_string += "      std::stringstream stream;\n"
        check_string += '      stream << "' + name + ' "'
        check_string += " << std::setfill('0') << std::setw(4)"
        check_string += ' << std::uppercase << std::hex << code_point;\n'
        check_string += '      return stream.str();\n'
        check_string += '    }'

        if check_string_metadata: check_string_metadata += " else "
        else: check_string_metadata = "    "
        check_string_metadata += "if (code_point >= " + hex(first_numeric_value) + " && code_point <= " + hex(last_numeric_value) + ") {\n"
        check_string_metadata += "      return &code_points_array[" + str(array_index) + "];\n"
        check_string_metadata += "    }"

    # http://www.unicode.org/reports/tr44/#General_Category_Values
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

    code_point_to_array_index[numeric_value] = array_index
    array_index += 1

print("};")
print("")
print("std::map<uint32_t, code_point> unicode_code_points;")
print("void unicode_code_points_initialize() {")
print("    for (unsigned int i = 0; i < sizeof(code_points_array) / sizeof(code_points_array[0]); i++) {")
print("        unicode_code_points[code_points_array[i].numeric_value] = code_points_array[i];")
print("    }")
print("}");
print("")
print("code_point const* lookup_code_point(uint32_t code_point) {")
print("  auto entry = unicode_code_points.find(code_point);")
print("  if (entry == unicode_code_points.end()) {")
print(check_string_metadata)
print("    fprintf(stderr, \"invalid code point %u\\n\", code_point);")
print("    exit(1);")
print("  }")
print("  return &entry->second;")
print("}")
print("")
print("std::string lookup_code_point_name(uint32_t code_point) {")
print("  auto entry = unicode_code_points.find(code_point);")
print("  if (entry == unicode_code_points.end()) {")
print(check_string)
print("    fprintf(stderr, \"Invalid code point %u\\n\", code_point);")
print("    exit(1);")
print("  }")
print("  return std::string(entry->second.name);")
print("}")
print("")

