#include "wordle_filter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace wordle {

namespace {

char to_lower_ascii(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

char to_upper_ascii(char ch) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
}

std::string normalize_word(const std::string& word) {
    std::string out(word.size(), '\0');
    std::transform(word.begin(), word.end(), out.begin(), to_lower_ascii);
    return out;
}

bool is_alpha_word(const std::string& word) {
    return std::all_of(word.begin(), word.end(), [](unsigned char c) {
        return std::isalpha(c) != 0;
    });
}

bool is_valid_feedback(const std::string& color) {
    return std::all_of(color.begin(), color.end(), [](char c) {
        const char u = to_upper_ascii(c);
        return u == 'G' || u == 'Y' || u == 'B';
    });
}

}  // namespace

std::set<std::string> load_dictionary(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open dictionary file: " + filepath);
    }

    std::set<std::string> dictionary;
    std::string line;
    while (std::getline(file, line)) {
        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        const auto last = line.find_last_not_of(" \t\r\n");
        const std::string token = line.substr(first, last - first + 1);

        if (token.size() != kWordLength || !is_alpha_word(token)) {
            throw std::invalid_argument(
                "Invalid dictionary entry (expected 5 alphabetic chars): " + token
            );
        }

        dictionary.insert(normalize_word(token));
    }

    return dictionary;
}

std::string evaluate_guess(const std::string& candidate, const std::string& guess) {
    if (candidate.size() != kWordLength || guess.size() != kWordLength) {
        throw std::invalid_argument("Candidate and guess must both be 5 characters.");
    }

    std::string feedback(kWordLength, 'B');
    std::array<int, 256> remaining{};

    // Pass 1: Lock exact positional matches ('G') and tally unmatched letters.
    for (std::size_t i = 0; i < kWordLength; ++i) {
        const char cand_ch = to_lower_ascii(candidate[i]);
        const char guess_ch = to_lower_ascii(guess[i]);
        if (cand_ch == guess_ch) {
            feedback[i] = 'G';
        } else {
            ++remaining[static_cast<unsigned char>(cand_ch)];
        }
    }

    // Pass 2: Match misplaced letters ('Y') against remaining available counts.
    for (std::size_t i = 0; i < kWordLength; ++i) {
        if (feedback[i] == 'G') {
            continue;
        }
        const auto ch = static_cast<unsigned char>(to_lower_ascii(guess[i]));
        if (remaining[ch] > 0) {
            feedback[i] = 'Y';
            --remaining[ch];
        }
    }

    return feedback;
}

std::vector<std::string> potential_guesses(
    std::string& target,
    const std::string& input,
    const std::string& color,
    std::set<std::string>& dictionary
) {
    if (target.size() != kWordLength ||
        input.size() != kWordLength ||
        color.size() != kWordLength) {
        throw std::invalid_argument("Target, input, and color must all be 5 characters.");
    }
    if (!is_alpha_word(input)) {
        throw std::invalid_argument("Guess input must contain only alphabetic characters.");
    }
    if (!is_valid_feedback(color)) {
        throw std::invalid_argument("Feedback color must contain only 'G', 'Y', or 'B'.");
    }

    std::string normalized_color(kWordLength, '\0');
    for (std::size_t i = 0; i < kWordLength; ++i) {
        normalized_color[i] = to_upper_ascii(color[i]);
        if (normalized_color[i] == 'G') {
            target[i] = to_lower_ascii(input[i]);
        }
    }

    // Single-pass in-place erasure avoids extra set allocations and O(log N) lookups.
    for (auto it = dictionary.begin(); it != dictionary.end(); ) {
        if (evaluate_guess(*it, input) != normalized_color) {
            it = dictionary.erase(it);
        } else {
            ++it;
        }
    }

    return {dictionary.begin(), dictionary.end()};
}

}  // namespace wordle
