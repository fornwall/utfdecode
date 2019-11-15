#ifndef NORMALIZATION_HPP_INCLUDED
#define NORMALIZATION_HPP_INCLUDED

enum class normalization_form_t {
  NONE, // Do not use a normalization form.
  NFD,  // Canonical Decomposition.
  NFC,  // Canonical Decomposition, followed by Canonical Composition.
  NFKD, // Compatibility Decomposition
  NFKC  // Compatibility Decomposition, followed by Canonical Composition
};

#endif
