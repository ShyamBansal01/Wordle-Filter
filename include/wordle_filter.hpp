#pragma once

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace wordle {

constexpr std::size_t kWordLength = 5;

/**
 * Loads a newline- or whitespace-delimited dictionary of 5-letter words from disk.
 * Normalizes words to lowercase and skips blank lines.
 *
 * @throws std::runtime_error if the file cannot be opened.
 * @throws std::invalid_argument if any non-empty entry is not 5 alphabetic characters.
 */
std::set<std::string> load_dictionary(const std::string& filepath);

/**
 * Computes the exact Wordle feedback string ('G', 'Y', 'B') for a given
 * guess assuming `candidate` is the secret target word.
 *
 * Time Complexity:  O(L), where L = kWordLength (5).
 * Space Complexity: O(1).
 *
 * @throws std::invalid_argument if either word is not `kWordLength` characters.
 */
std::string evaluate_guess(
    const std::string& candidate,
    const std::string& guess
);

/**
 * Updates `target` with newly discovered green ('G') letters, prunes invalid
 * candidates from `dictionary` in-place, and returns the remaining valid words.
 *
 * Time Complexity:  O(N * L), where N = dictionary size and L = kWordLength (5).
 * Space Complexity: O(1) auxiliary space (excluding the returned vector).
 *
 * @throws std::invalid_argument if inputs have invalid lengths or feedback chars.
 */
std::vector<std::string> potential_guesses(
    std::string& target,
    const std::string& input,
    const std::string& color,
    std::set<std::string>& dictionary
);

}  // namespace wordle
