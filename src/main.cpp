#include "wordle_filter.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::string dict_path = (argc > 1) ? argv[1] : "data/WORDS";

    try {
        auto dictionary = wordle::load_dictionary(dict_path);
        std::string target(wordle::kWordLength, '_');

        std::string guess;
        std::string color;
        while (std::cin >> guess) {
            if (guess == "quit" || guess == "exit") {
                break;
            }
            if (!(std::cin >> color)) {
                std::cerr << "Error: Missing feedback string.\n";
                return 1;
            }

            try {
                const auto candidates = wordle::potential_guesses(target, guess, color, dictionary);
                std::cout << candidates.size() << "\n";
                for (std::size_t i = 0; i < candidates.size(); ++i) {
                    std::cout << candidates[i] << (i + 1 < candidates.size() ? " " : "\n");
                }
                if (candidates.size() <= 1) {
                    break;
                }
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid input: " << e.what() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
