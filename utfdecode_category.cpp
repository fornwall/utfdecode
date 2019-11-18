#ifndef GENERAL_CATEGORY_VALUES_HPP_INCLUDED
#define GENERAL_CATEGORY_VALUES_HPP_INCLUDED

#include "utfdecode.hpp"

char const *general_category_description(general_category_value_t category) {
  switch (category) {
  case general_category_value_t::Uppercase_Letter:
    return "an uppercase letter";
  case general_category_value_t::Lowercase_Letter:
    return "a lowercase letter";
  case general_category_value_t::Titlecase_Letter:
    return "a digraphic character, with first part uppercase";
  case general_category_value_t::Cased_Letter:
    return "Lu | Ll | Lt";
  case general_category_value_t::Modifier_Letter:
    return "a modifier letter";
  case general_category_value_t::Other_Letter:
    return "other letters, including syllables and ideographs";
  case general_category_value_t::Letter:
    return "Lu | Ll | Lt | Lm | Lo";
  case general_category_value_t::Nonspacing_Mark:
    return "a nonspacing combining mark (zero advance width)";
  case general_category_value_t::Spacing_Mark:
    return "a spacing combining mark (positive advance width)";
  case general_category_value_t::Enclosing_Mark:
    return "an enclosing combining mark";
  case general_category_value_t::Mark:
    return "Mn | Mc | Me";
  case general_category_value_t::Decimal_Number:
    return "a decimal digit";
  case general_category_value_t::Letter_Number:
    return "a letterlike numeric character";
  case general_category_value_t::Other_Number:
    return "a numeric character of other type";
  case general_category_value_t::Number:
    return "Nd | Nl | No";
  case general_category_value_t::Connector_Punctuation:
    return "a connecting punctuation mark, like a tie";
  case general_category_value_t::Dash_Punctuation:
    return "a dash or hyphen punctuation mark";
  case general_category_value_t::Open_Punctuation:
    return "an opening punctuation mark (of a pair)";
  case general_category_value_t::Close_Punctuation:
    return "a closing punctuation mark (of a pair)";
  case general_category_value_t::Initial_Punctuation:
    return "an initial quotation mark";
  case general_category_value_t::Final_Punctuation:
    return "a final quotation mark";
  case general_category_value_t::Other_Punctuation:
    return "a punctuation mark of other type";
  case general_category_value_t::Punctuation:
    return "Pc | Pd | Ps | Pe | Pi | Pf | Po";
  case general_category_value_t::Math_Symbol:
    return "a symbol of mathematical use";
  case general_category_value_t::Currency_Symbol:
    return "a currency sign";
  case general_category_value_t::Modifier_Symbol:
    return "a non-letterlike modifier symbol";
  case general_category_value_t::Other_Symbol:
    return "a symbol of other type";
  case general_category_value_t::Symbol:
    return "Sm | Sc | Sk | Sok";
  case general_category_value_t::Space_Separator:
    return "a space character (of various non-zero widths)";
  case general_category_value_t::Line_Separator:
    return "U+2028 LINE SEPARATOR only";
  case general_category_value_t::Paragraph_Separator:
    return "U+2029 PARAGRAPH SEPARATOR only";
  case general_category_value_t::Separator:
    return "Zs | Zl | Zp";
  case general_category_value_t::Control:
    return "a C0 or C1 control code";
  case general_category_value_t::Format:
    return "a format control character";
  case general_category_value_t::Surrogate:
    return "a surrogate code point";
  case general_category_value_t::Private_Use:
    return "a private-use character";
  case general_category_value_t::Unassigned:
    return "a reserved unassigned code point or a noncharacter";
  case general_category_value_t::Other:
    return "Cc | Cf | Cs | Co | Cn ";
  default:
    return "TODO";
  }
}

bool general_category_is_combining(general_category_value_t category) {
	switch (category) {
	case general_category_value_t::Nonspacing_Mark:
	case general_category_value_t::Spacing_Mark:
	case general_category_value_t::Enclosing_Mark:
		return true;
	default:
		return false;
	}
}

#endif
