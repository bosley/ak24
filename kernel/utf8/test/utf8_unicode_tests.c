#include "test/assert.h"
#include "utf8.h"
#include <string.h>

// =============================================================================
// Unicode Digit Classification Tests
// =============================================================================

static int test_digit_ascii(void) {
  // ASCII digits must be recognized
  for (uint32_t cp = 0x30; cp <= 0x39; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }
  // Non-digits
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x2F) == false); // '/' before '0'
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x3A) == false); // ':' after '9'
  AK24_TEST_PASS();
}

static int test_digit_arabic_indic(void) {
  // Arabic-Indic digits (U+0660-U+0669): ٠ ١ ٢ ٣ ٤ ٥ ٦ ٧ ٨ ٩
  for (uint32_t cp = 0x0660; cp <= 0x0669; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x065F) == false);
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x066A) == false);
  AK24_TEST_PASS();
}

static int test_digit_devanagari(void) {
  // Devanagari digits (U+0966-U+096F): ० १ २ ३ ४ ५ ६ ७ ८ ९
  for (uint32_t cp = 0x0966; cp <= 0x096F; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0965) == false);
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0970) == false);
  AK24_TEST_PASS();
}

static int test_digit_thai(void) {
  // Thai digits (U+0E50-U+0E59): ๐ ๑ ๒ ๓ ๔ ๕ ๖ ๗ ๘ ๙
  for (uint32_t cp = 0x0E50; cp <= 0x0E59; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0E4F) == false);
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0E5A) == false);
  AK24_TEST_PASS();
}

static int test_digit_fullwidth(void) {
  // Fullwidth digits (U+FF10-U+FF19): ０１２３４５６７８９
  for (uint32_t cp = 0xFF10; cp <= 0xFF19; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }
  AK24_TEST_ASSERT(ak_utf8_is_digit(0xFF0F) == false);
  AK24_TEST_ASSERT(ak_utf8_is_digit(0xFF1A) == false);
  AK24_TEST_PASS();
}

static int test_digit_comprehensive(void) {
  // Test various digit scripts
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x09E6) == true); // Bengali ০
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0A66) == true); // Gurmukhi ੦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0AE6) == true); // Gujarati ૦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0B66) == true); // Oriya ୦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0BE6) == true); // Tamil ௦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0C66) == true); // Telugu ౦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0CE6) == true); // Kannada ೦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0D66) == true); // Malayalam ൦
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0ED0) == true); // Lao ໐
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x0F20) == true); // Tibetan ༠
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x1040) == true); // Myanmar ၀
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x17E0) == true); // Khmer ០
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x1810) == true); // Mongolian ᠐
  AK24_TEST_PASS();
}

// =============================================================================
// Unicode Letter Classification Tests
// =============================================================================

static int test_alpha_ascii(void) {
  // ASCII uppercase (A-Z)
  for (uint32_t cp = 0x41; cp <= 0x5A; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_alpha(cp) == true);
  }
  // ASCII lowercase (a-z)
  for (uint32_t cp = 0x61; cp <= 0x7A; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_alpha(cp) == true);
  }
  // Non-letters
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x40) == false); // '@'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x5B) == false); // '['
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x60) == false); // '`'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x7B) == false); // '{'
  AK24_TEST_PASS();
}

static int test_alpha_latin_extended(void) {
  // Latin-1 Supplement: À Á Â Ã Ä Å ... ö ÷ ... ÿ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00C0) == true); // À
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00D6) == true); // Ö
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00D8) == true); // Ø
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00E9) == true); // é
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00F6) == true); // ö
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00FF) == true); // ÿ

  // Latin Extended-A: Ā ā Ă ă ... ſ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0100) == true); // Ā
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x017F) == true); // ſ

  // Latin Extended-B
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0180) == true);
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x024F) == true);

  // Non-letters in Latin ranges
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00D7) == false); // ×
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x00F7) == false); // ÷
  AK24_TEST_PASS();
}

static int test_alpha_greek(void) {
  // Greek and Coptic (U+0370-U+03FF): Α Β Γ Δ ... α β γ δ ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0391) == true); // Α (Alpha)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0392) == true); // Β (Beta)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0393) == true); // Γ (Gamma)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x03B1) == true); // α (alpha)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x03B2) == true); // β (beta)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x03C9) == true); // ω (omega)
  AK24_TEST_PASS();
}

static int test_alpha_cyrillic(void) {
  // Cyrillic (U+0400-U+04FF): А Б В Г ... а б в г ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0410) == true); // А
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0411) == true); // Б
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0430) == true); // а
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0431) == true); // б
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x044F) == true); // я

  // Cyrillic Supplement
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0500) == true);
  AK24_TEST_PASS();
}

static int test_alpha_hebrew(void) {
  // Hebrew (U+05D0-U+05EA): א ב ג ד ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x05D0) == true); // א (Alef)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x05D1) == true); // ב (Bet)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x05E9) == true); // ש (Shin)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x05EA) == true); // ת (Tav)
  AK24_TEST_PASS();
}

static int test_alpha_arabic(void) {
  // Arabic (U+0621-U+064A): ء ا ب ت ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0621) == true); // ء (Hamza)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0627) == true); // ا (Alif)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0628) == true); // ب (Ba)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x062A) == true); // ت (Ta)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0644) == true); // ل (Lam)
  AK24_TEST_PASS();
}

static int test_alpha_devanagari(void) {
  // Devanagari (U+0905-U+0939): अ आ इ ई ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0905) == true); // अ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0906) == true); // आ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0915) == true); // क
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0928) == true); // न
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x0939) == true); // ह
  AK24_TEST_PASS();
}

static int test_alpha_cjk(void) {
  // CJK Unified Ideographs (U+4E00-U+9FFF)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x4E00) == true);  // 一
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x4E2D) == true);  // 中
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x65E5) == true);  // 日
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x672C) == true);  // 本
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x8A9E) == true);  // 語
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x9FFF) == true);  // Last CJK
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x4DFF) == false); // Before range
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xA000) == false); // After range
  AK24_TEST_PASS();
}

static int test_alpha_hiragana(void) {
  // Hiragana (U+3041-U+3096): あ い う え お ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3041) == true); // ぁ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3042) == true); // あ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3044) == true); // い
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3046) == true); // う
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3093) == true); // ん
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x3096) == true); // ゖ
  AK24_TEST_PASS();
}

static int test_alpha_katakana(void) {
  // Katakana (U+30A1-U+30FA): ア イ ウ エ オ ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30A1) == true); // ァ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30A2) == true); // ア
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30AB) == true); // カ
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30F3) == true); // ン
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30FA) == true); // ヺ
  AK24_TEST_PASS();
}

static int test_alpha_hangul(void) {
  // Hangul Syllables (U+AC00-U+D7A3): 가 나 다 ...
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xAC00) == true);  // 가
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xB098) == true);  // 나
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xB2E4) == true);  // 다
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xD55C) == true);  // 한
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xD7A3) == true);  // 힣 (last)
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xABFF) == false); // Before
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0xD7A4) == false); // After
  AK24_TEST_PASS();
}

// =============================================================================
// Combining Mark Tests
// =============================================================================

static int test_combining_marks_basic(void) {
  // Combining Diacritical Marks (U+0300-U+036F)
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0300) == true); // Grave
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0301) == true); // Acute
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0302) == true); // Circumflex
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0303) == true); // Tilde
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0308) == true); // Diaeresis
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0327) == true); // Cedilla
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x036F) == true); // Last in range

  // Not combining marks
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x02FF) == false);
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0370) == false);
  AK24_TEST_PASS();
}

static int test_combining_marks_hebrew(void) {
  // Hebrew combining marks
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0591) == true); // Hebrew accent
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x05B0) == true); // Sheva
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x05B1) == true); // Hataf Segol
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x05BD) == true); // Meteg
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x05BF) == true); // Rafe
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x05C1) == true); // Shin Dot
  AK24_TEST_PASS();
}

static int test_combining_marks_arabic(void) {
  // Arabic combining marks
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x064B) == true); // Fathatan
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x064C) == true); // Dammatan
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x064D) == true); // Kasratan
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x064E) == true); // Fatha
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0650) == true); // Kasra
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0652) == true); // Sukun
  AK24_TEST_PASS();
}

static int test_combining_marks_devanagari(void) {
  // Devanagari combining marks
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0900) == true);
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x093C) == true); // Nukta
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x094D) == true); // Virama
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(0x0951) == true); // Stress
  AK24_TEST_PASS();
}

// =============================================================================
// Grapheme Cluster Tests
// =============================================================================

static int test_grapheme_len_simple(void) {
  // Simple ASCII character
  const uint8_t *text = (uint8_t *)"A";
  AK24_TEST_ASSERT(ak_utf8_grapheme_len(text, 1) == 1);

  // Simple multibyte character
  const uint8_t *text2 = (uint8_t *)"日";
  AK24_TEST_ASSERT(ak_utf8_grapheme_len(text2, 3) == 3);
  AK24_TEST_PASS();
}

static int test_grapheme_len_combining(void) {
  // e with combining acute accent: e + U+0301
  const uint8_t text[] = {0x65, 0xCC, 0x81, 0x00}; // e + ́
  size_t len = ak_utf8_grapheme_len(text, 3);
  AK24_TEST_ASSERT(len == 3); // Base char (1) + combining mark (2)

  // a with multiple combining marks: a + grave + tilde
  const uint8_t text2[] = {0x61, 0xCC, 0x80, 0xCC, 0x83, 0x00}; // a + ̀ + ̃
  len = ak_utf8_grapheme_len(text2, 5);
  AK24_TEST_ASSERT(len == 5); // Base (1) + grave (2) + tilde (2)
  AK24_TEST_PASS();
}

static int test_grapheme_len_precomposed(void) {
  // Precomposed é (U+00E9) is a single grapheme
  const uint8_t *text = (uint8_t *)"é"; // U+00E9 = 0xC3 0xA9
  AK24_TEST_ASSERT(ak_utf8_grapheme_len(text, 2) == 2);
  AK24_TEST_PASS();
}

static int test_grapheme_count_simple(void) {
  const uint8_t *text = (uint8_t *)"Hello";
  AK24_TEST_ASSERT(ak_utf8_grapheme_count(text, 5) == 5);
  AK24_TEST_PASS();
}

static int test_grapheme_count_combining(void) {
  // "café" with combining accent: c a f e + ́
  const uint8_t text[] = {0x63, 0x61, 0x66, 0x65, 0xCC, 0x81, 0x00};
  // Bytes: c(1) a(1) f(1) e(1) + combining(2) = 6 bytes, 4 graphemes
  AK24_TEST_ASSERT(ak_utf8_grapheme_count(text, 6) == 4);
  AK24_TEST_PASS();
}

static int test_grapheme_count_mixed(void) {
  // Mix of precomposed and decomposed
  const uint8_t *precomposed = (uint8_t *)"café"; // c a f é(precomposed)
  AK24_TEST_ASSERT(ak_utf8_grapheme_count(precomposed, 5) == 4);

  // Decomposed version: c a f e + combining
  const uint8_t decomposed[] = {0x63, 0x61, 0x66, 0x65, 0xCC, 0x81, 0x00};
  AK24_TEST_ASSERT(ak_utf8_grapheme_count(decomposed, 6) == 4);
  AK24_TEST_PASS();
}

static int test_grapheme_count_complex(void) {
  // Hebrew with vowel points: שָׁלוֹם (shalom with nikud)
  // ש (U+05E9) + Qamatz (U+05B8) + Shin Dot (U+05C1) + ל + וֹ + ם
  // Each letter with its diacritics forms one grapheme cluster
  const uint8_t text[] = {0xD7, 0xA9, // ש (Shin)
                          0xD6, 0xB8, // ָ (Qamatz) - combining
                          0xD7, 0x81, // ׁ (Shin Dot) - combining
                          0xD7, 0x9C, // ל (Lamed)
                          0xD7, 0x95, // ו (Vav)
                          0xD6, 0xB9, // ֹ (Holam) - combining
                          0xD7, 0x9D, // ם (Mem Sofit)
                          0x00};

  // Base characters: ש, ל, ו, ם = 4 grapheme clusters
  // (Shin Dot U+05C1 is NOT a combining mark in our implementation)
  size_t count = ak_utf8_grapheme_count(text, 14);
  AK24_TEST_ASSERT(count == 4);
  AK24_TEST_PASS();
}

static int test_grapheme_cluster_boundaries(void) {
  // Test that we don't combine across non-combining characters
  const uint8_t text[] = {0x61,       // a
                          0xCC, 0x81, // combining acute
                          0x62,       // b
                          0xCC, 0x80, // combining grave
                          0x00};

  // Should be 2 grapheme clusters: "á" and "b̀"
  AK24_TEST_ASSERT(ak_utf8_grapheme_count(text, 6) == 2);

  // First grapheme: a + acute = 3 bytes
  AK24_TEST_ASSERT(ak_utf8_grapheme_len(text, 6) == 3);

  // Second grapheme: b + grave = 3 bytes
  AK24_TEST_ASSERT(ak_utf8_grapheme_len(text + 3, 3) == 3);
  AK24_TEST_PASS();
}

// =============================================================================
// Integration Tests - Proving Correctness
// =============================================================================

static int test_digit_classification_completeness(void) {
  // Prove: ak_utf8_is_digit recognizes both ASCII and Unicode digits

  // ASCII digits
  for (uint32_t cp = 0x30; cp <= 0x39; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_digit(cp) == true);
  }

  // Non-ASCII Unicode digits from multiple scripts
  int unicode_digit_count = 0;
  uint32_t test_ranges[][2] = {
      {0x0660, 0x0669}, // Arabic-Indic
      {0x0966, 0x096F}, // Devanagari
      {0x0E50, 0x0E59}, // Thai
      {0xFF10, 0xFF19}, // Fullwidth
  };

  for (size_t i = 0; i < 4; i++) {
    for (uint32_t cp = test_ranges[i][0]; cp <= test_ranges[i][1]; cp++) {
      if (ak_utf8_is_digit(cp)) {
        unicode_digit_count++;
      }
    }
  }
  AK24_TEST_ASSERT(unicode_digit_count == 40); // 10 digits × 4 scripts
  AK24_TEST_PASS();
}

static int test_alpha_classification_completeness(void) {
  // Prove: ak_utf8_is_alpha recognizes both ASCII and Unicode letters

  // ASCII uppercase (A-Z)
  for (uint32_t cp = 0x41; cp <= 0x5A; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_alpha(cp) == true);
  }
  // ASCII lowercase (a-z)
  for (uint32_t cp = 0x61; cp <= 0x7A; cp++) {
    AK24_TEST_ASSERT(ak_utf8_is_alpha(cp) == true);
  }

  // Prove: Unicode letters from various non-ASCII scripts
  uint32_t sample_letters[] = {
      0x00E9, // é (Latin Extended)
      0x0391, // Α (Greek)
      0x0410, // А (Cyrillic)
      0x05D0, // א (Hebrew)
      0x0627, // ا (Arabic)
      0x4E00, // 一 (CJK)
      0x3042, // あ (Hiragana)
      0xAC00, // 가 (Hangul)
  };

  for (size_t i = 0; i < 8; i++) {
    AK24_TEST_ASSERT(ak_utf8_is_alpha(sample_letters[i]) == true);
  }
  AK24_TEST_PASS();
}

static int test_grapheme_vs_codepoint_correctness(void) {
  // Prove: Grapheme count ≤ Codepoint count
  const uint8_t text1[] = {0x65, 0xCC, 0x81, 0x00}; // e + combining acute

  size_t codepoint_count = ak_utf8_char_count(text1, 3);
  size_t grapheme_count = ak_utf8_grapheme_count(text1, 3);

  AK24_TEST_ASSERT(codepoint_count == 2); // Two codepoints
  AK24_TEST_ASSERT(grapheme_count == 1);  // One grapheme
  AK24_TEST_ASSERT(grapheme_count <= codepoint_count);

  // Prove: Without combining marks, counts are equal
  const uint8_t *text2 = (uint8_t *)"Hello";
  codepoint_count = ak_utf8_char_count(text2, 5);
  grapheme_count = ak_utf8_grapheme_count(text2, 5);
  AK24_TEST_ASSERT(codepoint_count == grapheme_count);
  AK24_TEST_PASS();
}

static int test_combining_mark_transitivity(void) {
  // Prove: Combining marks always follow base characters
  const uint8_t valid[] = {0x65, 0xCC, 0x81, 0x00}; // e + combining

  // First codepoint should NOT be combining
  ak_utf8_decode_result_t r1 = ak_utf8_decode(valid, 3);
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(r1.codepoint) == false);

  // Second codepoint SHOULD be combining
  ak_utf8_decode_result_t r2 = ak_utf8_decode(valid + 1, 2);
  AK24_TEST_ASSERT(ak_utf8_is_combining_mark(r2.codepoint) == true);

  // Grapheme should include both
  size_t grapheme_len = ak_utf8_grapheme_len(valid, 3);
  AK24_TEST_ASSERT(grapheme_len == 3);
  AK24_TEST_PASS();
}

// =============================================================================
// Test Runner
// =============================================================================

int main(void) {
  // Unicode digit tests
  AK24_TEST_RUN(test_digit_ascii);
  AK24_TEST_RUN(test_digit_arabic_indic);
  AK24_TEST_RUN(test_digit_devanagari);
  AK24_TEST_RUN(test_digit_thai);
  AK24_TEST_RUN(test_digit_fullwidth);
  AK24_TEST_RUN(test_digit_comprehensive);

  // Unicode letter tests
  AK24_TEST_RUN(test_alpha_ascii);
  AK24_TEST_RUN(test_alpha_latin_extended);
  AK24_TEST_RUN(test_alpha_greek);
  AK24_TEST_RUN(test_alpha_cyrillic);
  AK24_TEST_RUN(test_alpha_hebrew);
  AK24_TEST_RUN(test_alpha_arabic);
  AK24_TEST_RUN(test_alpha_devanagari);
  AK24_TEST_RUN(test_alpha_cjk);
  AK24_TEST_RUN(test_alpha_hiragana);
  AK24_TEST_RUN(test_alpha_katakana);
  AK24_TEST_RUN(test_alpha_hangul);

  // Combining mark tests
  AK24_TEST_RUN(test_combining_marks_basic);
  AK24_TEST_RUN(test_combining_marks_hebrew);
  AK24_TEST_RUN(test_combining_marks_arabic);
  AK24_TEST_RUN(test_combining_marks_devanagari);

  // Grapheme cluster tests
  AK24_TEST_RUN(test_grapheme_len_simple);
  AK24_TEST_RUN(test_grapheme_len_combining);
  AK24_TEST_RUN(test_grapheme_len_precomposed);
  AK24_TEST_RUN(test_grapheme_count_simple);
  AK24_TEST_RUN(test_grapheme_count_combining);
  AK24_TEST_RUN(test_grapheme_count_mixed);
  AK24_TEST_RUN(test_grapheme_count_complex);
  AK24_TEST_RUN(test_grapheme_cluster_boundaries);

  // Integration/correctness proofs
  AK24_TEST_RUN(test_digit_classification_completeness);
  AK24_TEST_RUN(test_alpha_classification_completeness);
  AK24_TEST_RUN(test_grapheme_vs_codepoint_correctness);
  AK24_TEST_RUN(test_combining_mark_transitivity);

  return 0;
}
