Files from http://www.unicode.org/Public/7.0.0/ucd/

Sample
======
0000;<control>;Cc;0;BN;;;;;N;NULL;;;;
0001;<control>;Cc;0;BN;;;;;N;START OF HEADING;;;;

Second column
=============
Property Type	Symbol		Examples
Catalog		C		Age, Block
Enumeration	E		Joining_Type, Line_Break
Binary		B		Uppercase, White_Space
String		S		Uppercase_Mapping, Case_Folding
Numeric		N		Numeric_Value
Miscellaneous	M		Name, Jamo_Short_Name

where
- Catalog properties have enumerated values which are expected to be regularly extended in successive versions of the Unicode Standard. This distinguishes them from Enumeration properties.
- Enumeration properties have enumerated values which constitute a logical partition space; new values will generally not be added to them in successive versions of the standard.
- Binary properties are a special case of Enumeration properties, which have exactly two values: Yes and No (or True and False).
- String properties are typically mappings from a Unicode code point to another Unicode code point or sequence of Unicode code points; examples include case mappings and decomposition mappings.
- Numeric properties specify the actual numeric values for digits and other characters associated with numbers in some way.
- Miscellaneous properties are those properties that do not fit neatly into the other property categories; they currently include character names, comments about characters, the Script_Extensions property, and the Unicode_Radical_Stroke property (a combination of numeric values) documented in Unicode Standard Annex #38, "Unicode Han Database (Unihan)" [UAX38].

Format
======
Format described at: http://www.unicode.org/reports/tr44/#UnicodeData.txt

- Name (Miscellaneous)                   (1) These names match exactly the names published in the code charts of the Unicode Standard. The derived Hangul Syllable names are omitted from this file; see Jamo.txt for their derivation.

- General_Category (Enumeration)         (2) This is a useful breakdown into various character types which can be used as a default categorization in implementations. For the property values, see General Category Values.

- Canonical_Combining_Class (Numeric)    (3) The classes used for the Canonical Ordering Algorithm in the Unicode Standard. This property could be considered either an enumerated property or a numeric property:
                                         the principal use of the property is in terms of the numeric values. For the property value names associated with different numeric values, see DerivedCombiningClass.txt and Canonical Combining Class Values.

- Bidi_Class (Enumeration)               (4) These are the categories required by the Unicode Bidirectional Algorithm. For the property values, see Bidirectional Class Values. For more information,
                                         see Unicode Standard Annex #9, "Unicode Bidirectional Algorithm" [UAX9].
					 The default property values depend on the code point, and are explained in DerivedBidiClass.txt

-Decomposition_Type (Enumeration,String),
 Decomposition_Mapping                   (5) This field contains both values, with the type in angle brackets. The decomposition mappings exactly match the decomposition mappings published with the character names in
                                         the Unicode Standard. For more information, see Character Decomposition Mappings.

- Numeric_Type (Enumeration,Numeric)
  Numeric_Value                          (6) If the character has the property value Numeric_Type=Decimal, then the Numeric_Value of that digit is represented with an integer value (limited to the range 0..9) in fields 6, 7, and 8. Characters with the property value Numeric_Type=Decimal are restricted to digits which can be used in a decimal radix positional numeral system and which are encoded in the standard in a contiguous ascending range 0..9. See the discussion of decimal digits in Chapter 4, Character Properties in [Unicode].
                                         (7) If the character has the property value Numeric_Type=Digit, then the Numeric_Value of that digit is represented with an integer value (limited to the range 0..9) in fields 7 and 8, and field 6 is null. This covers digits that need special handling, such as the compatibility superscript digits.
Starting with Unicode 6.3.0, no newly encoded numeric characters will be given Numeric_Type=Digit, nor will existing characters with Numeric_Type=Numeric be changed to Numeric_Type=Digit. The distinction between those two types is not considered useful.
                                         (8) If the character has the property value Numeric_Type=Numeric, then the Numeric_Value of that character is represented with a positive or negative integer or rational number in this field, and fields 6 and 7 are null. This includes fractions such as, for example, "1/5" for U+2155 VULGAR FRACTION ONE FIFTH.
Some characters have these properties based on values from the Unihan data files. See Numeric_Type, Han.

- Bidi_Mirrored (Binary)                 (9) If the character is a "mirrored" character in bidirectional text, this field has the value "Y"; otherwise "N". See Section 4.7, Bidi Mirrored of [Unicode]. Do not confuse this with the Bidi_Mirroring_Glyph property.

- Unicode_1_Name (Obsolete as of 6.2.0)  (10) Old name as published in Unicode 1.0 or ISO 6429 names for control functions. This field is empty unless it is significantly different from the current name for the character. No longer used in code chart production. See Name_Alias.

- ISO_Comment (Obsolete as of 5.2.0; Deprecated and Stabilized as of 6.0.0) (11) ISO 10646 comment field. It was used for notes that appeared in parentheses in the 10646 names list, or contained an asterisk to mark an Annex P note.
As of Unicode 5.2.0, this field no longer contains any non-null values.

- Simple_Uppercase_Mapping (String)      (12) Simple uppercase mapping (single character result). If a character is part of an alphabet with case distinctions, and has a simple uppercase equivalent, then the
                                       uppercase equivalent is in this field. The simple mappings have a single character result, where the full mappings may have multi-character results. For more information, see Case and Case Mapping.

- Simple_Lowercase_Mapping (String)      (13) Simple lowercase mapping (single character result).

- Simple_Titlecase_Mapping (String)      (14) Simple titlecase mapping (single character result).
                                         Note: If this field is null, then the Simple_Titlecase_Mapping is the same as the Simple_Uppercase_Mapping for this character.

