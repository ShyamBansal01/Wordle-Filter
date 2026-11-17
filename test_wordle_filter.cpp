#include "wordle_filter.hpp"

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void fail_assertion(const char* expr, const char* file, int line, const std::string& detail = "") {
    std::cerr << "  [FAIL] " << file << ":" << line << " -> " << expr;
    if (!detail.empty()) {
        std::cerr << " (" << detail << ")";
    }
    std::cerr << "\n";
    throw std::runtime_error("Assertion failed");
}

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            fail_assertion(#expr, __FILE__, __LINE__); \
        } \
    } while (false)

#define ASSERT_EQ(actual, expected) \
    do { \
        const auto& a_val = (actual); \
        const auto& e_val = (expected); \
        if (!(a_val == e_val)) { \
            std::ostringstream oss; \
            oss << "expected \"" << e_val << "\", got \"" << a_val << "\""; \
            fail_assertion(#actual " == " #expected, __FILE__, __LINE__, oss.str()); \
        } \
    } while (false)

#define ASSERT_VEC_EQ(actual, expected) \
    do { \
        const auto& a_vec = (actual); \
        const auto& e_vec = (expected); \
        if (a_vec != e_vec) { \
            fail_assertion(#actual " == " #expected, __FILE__, __LINE__, "vector contents differ"); \
        } \
    } while (false)

#define ASSERT_THROWS(stmt, exc_type) \
    do { \
        bool caught = false; \
        try { \
            stmt; \
        } catch (const exc_type&) { \
            caught = true; \
        } \
        if (!caught) { \
            fail_assertion(#stmt " throws " #exc_type, __FILE__, __LINE__); \
        } \
    } while (false)

class TempFile {
public:
    TempFile(const std::string& filename, const std::string& content)
        : path_(".tmp_" + filename) {
        std::ofstream out(path_, std::ios::binary);
        out << content;
    }

    ~TempFile() {
        std::remove(path_.c_str());
    }

    const std::string& str() const { return path_; }

private:
    std::string path_;
};

void run_test(const std::string& name, const std::function<void()>& fn) {
    try {
        fn();
        ++g_passed;
        std::cout << "[PASS] " << name << "\n";
    } catch (...) {
        ++g_failed;
        std::cout << "[FAIL] " << name << "\n";
    }
}

// --- File Loading Tests ---

void test_load_dictionary_valid_and_crlf() {
    TempFile tmp(
        "wordle_test_valid.txt",
        "aback\r\n"
        "  ADIEU  \n"
        "\n"
        "storm\r\n"
        "aback\n"
    );

    const auto dict = wordle::load_dictionary(tmp.str());
    ASSERT_EQ(dict.size(), 3u);
    ASSERT_TRUE(dict.count("aback") == 1);
    ASSERT_TRUE(dict.count("adieu") == 1);
    ASSERT_TRUE(dict.count("storm") == 1);
}

void test_load_dictionary_missing_file_throws() {
    ASSERT_THROWS(
        (void)wordle::load_dictionary("nonexistent_wordle_dict_12345.txt"),
        std::runtime_error
    );
}

void test_load_dictionary_malformed_entries_throw() {
    TempFile short_word("wordle_test_short.txt", "aback\ncat\nstorm\n");
    ASSERT_THROWS((void)wordle::load_dictionary(short_word.str()), std::invalid_argument);

    TempFile non_alpha("wordle_test_nonalpha.txt", "aback\nabc12\nstorm\n");
    ASSERT_THROWS((void)wordle::load_dictionary(non_alpha.str()), std::invalid_argument);
}

// --- Core Evaluation & Filtering Edge Cases ---

void test_evaluate_guess_basic() {
    ASSERT_EQ(wordle::evaluate_guess("apple", "apple"), "GGGGG");
    ASSERT_EQ(wordle::evaluate_guess("apple", "brick"), "BBBBB");
    ASSERT_EQ(wordle::evaluate_guess("apple", "pleap"), "YYYYY");
}

void test_evaluate_guess_duplicate_letters() {
    ASSERT_EQ(wordle::evaluate_guess("apple", "peeps"), "YYBYB");
    ASSERT_EQ(wordle::evaluate_guess("apple", "eagle"), "BYBGG");
    ASSERT_EQ(wordle::evaluate_guess("eerie", "geese"), "BGYBG");
}

void test_duplicate_gray_does_not_eliminate_valid_word() {
    std::string target = "_____";
    std::set<std::string> dict = {"apple", "peeps", "grape", "mango", "spare"};

    const auto result = wordle::potential_guesses(target, "peeps", "YYBYB", dict);

    ASSERT_EQ(target, "_____");
    ASSERT_VEC_EQ(result, (std::vector<std::string>{"apple"}));
    ASSERT_EQ(dict.size(), 1u);
}

void test_green_does_not_double_satisfy_yellow() {
    std::string target = "_____";
    std::set<std::string> dict = {"eerie", "perch", "tease", "genie"};

    const auto result = wordle::potential_guesses(target, "geese", "BGYBG", dict);

    ASSERT_EQ(target, "_e__e");
    ASSERT_VEC_EQ(result, (std::vector<std::string>{"eerie"}));
}

void test_yellow_excludes_same_position() {
    std::string target = "_____";
    std::set<std::string> dict = {"crane", "trace", "react", "cater"};

    const auto result = wordle::potential_guesses(target, "cloud", "YBBBB", dict);

    ASSERT_VEC_EQ(result, (std::vector<std::string>{"react", "trace"}));
}

void test_multi_turn_game_simulation() {
    std::string target = "_____";
    std::set<std::string> dict = {"slate", "crate", "crane", "trace", "brave", "blink"};

    const auto turn1 = wordle::potential_guesses(target, "slate", "BBGBG", dict);
    ASSERT_EQ(target, "__a_e");
    ASSERT_VEC_EQ(turn1, (std::vector<std::string>{"brave", "crane"}));

    const auto turn2 = wordle::potential_guesses(target, "brave", "BGGBG", dict);
    ASSERT_EQ(target, "_ra_e");
    ASSERT_VEC_EQ(turn2, (std::vector<std::string>{"crane"}));

    const auto turn3 = wordle::potential_guesses(target, "crane", "GGGGG", dict);
    ASSERT_EQ(target, "crane");
    ASSERT_VEC_EQ(turn3, (std::vector<std::string>{"crane"}));
}

void test_case_insensitivity() {
    std::string target = "_____";
    std::set<std::string> dict = {"apple", "peeps", "mango"};

    const auto result = wordle::potential_guesses(target, "PEEPS", "yybyb", dict);
    ASSERT_VEC_EQ(result, (std::vector<std::string>{"apple"}));
}

void test_empty_and_no_match_dictionary() {
    std::string target = "_____";
    std::set<std::string> empty_dict;
    ASSERT_TRUE(wordle::potential_guesses(target, "crane", "BBBBB", empty_dict).empty());

    std::set<std::string> no_match_dict = {"crane", "slate"};
    ASSERT_TRUE(wordle::potential_guesses(target, "crane", "BBBBB", no_match_dict).empty());
    ASSERT_TRUE(no_match_dict.empty());
}

void test_invalid_inputs_throw() {
    std::string target = "_____";
    std::set<std::string> dict = {"crane"};

    ASSERT_THROWS((void)wordle::evaluate_guess("app", "apple"), std::invalid_argument);
    ASSERT_THROWS((void)wordle::evaluate_guess("apple", "apples"), std::invalid_argument);
    ASSERT_THROWS(wordle::potential_guesses(target, "cran", "BBBB", dict), std::invalid_argument);

    std::string short_target = "___";
    ASSERT_THROWS(wordle::potential_guesses(short_target, "crane", "BBBBB", dict), std::invalid_argument);

    ASSERT_THROWS(wordle::potential_guesses(target, "cran1", "BBBBB", dict), std::invalid_argument);
    ASSERT_THROWS(wordle::potential_guesses(target, "crane", "BBXBB", dict), std::invalid_argument);
}

void test_words_file_integration_if_present() {
    std::ifstream probe("data/WORDS");
    if (!probe.good()) {
        return;
    }
    probe.close();

    auto dict = wordle::load_dictionary("data/WORDS");
    ASSERT_TRUE(!dict.empty());

    std::string target = "_____";
    (void)wordle::potential_guesses(target, "adieu", "YBYBB", dict);
    const auto after_second = wordle::potential_guesses(target, "storm", "BBBBB", dict);
    ASSERT_TRUE(!after_second.empty());
    for (const auto& w : after_second) {
        ASSERT_EQ(w.size(), 5u);
    }
}

}  // namespace

int main() {
    run_test("load_dictionary: valid file, whitespace, CRLF, deduplication", test_load_dictionary_valid_and_crlf);
    run_test("load_dictionary: throws on missing file", test_load_dictionary_missing_file_throws);
    run_test("load_dictionary: throws on malformed entries", test_load_dictionary_malformed_entries_throw);
    run_test("evaluate_guess: basic G/Y/B", test_evaluate_guess_basic);
    run_test("evaluate_guess: duplicate letters", test_evaluate_guess_duplicate_letters);
    run_test("potential_guesses: duplicate gray regression", test_duplicate_gray_does_not_eliminate_valid_word);
    run_test("potential_guesses: green + yellow count enforcement", test_green_does_not_double_satisfy_yellow);
    run_test("potential_guesses: yellow position exclusion", test_yellow_excludes_same_position);
    run_test("potential_guesses: multi-turn game simulation", test_multi_turn_game_simulation);
    run_test("potential_guesses: case insensitivity", test_case_insensitivity);
    run_test("potential_guesses: empty & no-match dictionary", test_empty_and_no_match_dictionary);
    run_test("input validation: throws on invalid lengths/chars", test_invalid_inputs_throw);
    run_test("integration: data/WORDS file filtering", test_words_file_integration_if_present);

    std::cout << "\nSummary: " << g_passed << " passed, " << g_failed << " failed.\n";
    return g_failed == 0 ? 0 : 1;
}