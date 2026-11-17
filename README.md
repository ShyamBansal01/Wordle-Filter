# Wordle Candidate Filter & Solver

A modular, high-performance C++ library and CLI utility that filters a dictionary of 5-letter candidate words based on standard Wordle feedback (`G` = Green, `Y` = Yellow, `B` = Gray/Black).

## 1. Project Architecture

To make the codebase testable, maintainable, and production-ready, the application is decoupled into a reusable core library, a thin CLI entry point, and an automated unit test suite:

```text
Cred/
├── Makefile                    # Cross-platform build, test, and clean automation
├── README.md                   # System and algorithm documentation
├── data/
│   └── WORDS                   # Newline-delimited 5-letter dictionary file
├── include/
│   └── wordle_filter.hpp       # Public API declarations and contracts
├── src/
│   ├── wordle_filter.cpp       # Core dictionary loader, evaluator, and filter logic
│   └── main.cpp                # Interactive CLI driver (stdin/stdout)
└── tests/
    └── test_wordle_filter.cpp  # Standalone 13-test automated verification suite
```

### Component Responsibilities

* **`include/wordle_filter.hpp` & `src/wordle_filter.cpp`:** Pure, stateless/in-place domain logic with zero I/O coupling to `std::cin` or `std::cout`. Throws descriptive `std::invalid_argument` or `std::runtime_error` exceptions on invalid inputs.
* **`src/main.cpp`:** Handles command-line arguments, loads `data/WORDS` via `wordle::load_dictionary()`, and loops over user guesses and feedback strings.
* **`tests/test_wordle_filter.cpp`:** Zero-dependency test runner validating file parsing, duplicate-letter edge cases, multi-turn filtering, case insensitivity, and error handling.

## 2. How It Works Under the Hood

### The Core Insight: Forward Simulation

A common pitfall in Wordle solvers is maintaining separate global sets of "banned letters" (`restricted`) and "required misplaced letters" (`potential`). That approach breaks whenever a guess contains duplicate letters (for example, one copy turns Green or Yellow while the second copy turns Gray).

Instead, this implementation uses **forward equivalence simulation**:

> A dictionary word `candidate` can still be the secret answer **if and only if** scoring `guess` against `candidate` produces the exact same feedback string `color` observed in the game:
>
> $$\text{evaluate\_guess}(\text{candidate}, \text{guess}) == \text{color}$$

By simulating Wordle's exact scoring rules on each candidate, every duplicate-letter rule is enforced automatically without special-case branching.

### The Two-Pass `evaluate_guess` Algorithm

`wordle::evaluate_guess(candidate, guess)` computes the 5-character feedback string in two linear passes using a stack-allocated frequency table (`std::array<int, 256>`):

1. **Initialization:** Create a 5-character feedback string filled with `'B'` (Gray) and a zero-initialized frequency array `remaining[256]`.
2. **Pass 1 — Lock Exact Matches (`'G'`):**
   * Iterate $i \in \{0, 1, 2, 3, 4\}$.
   * If `candidate[i] == guess[i]`, set `feedback[i] = 'G'`.
   * Otherwise, increment `remaining[candidate[i]]` to record that this letter in `candidate` is still available to match a misplaced letter later.
3. **Pass 2 — Assign Misplaced Matches (`'Y'`):**
   * Iterate $i \in \{0, 1, 2, 3, 4\}$.
   * Skip positions already marked `'G'`.
   * If `remaining[guess[i]] > 0`, set `feedback[i] = 'Y'` and decrement `remaining[guess[i]]` so each letter in `candidate` can satisfy at most one Yellow tile.

#### Step-by-Step Edge-Case Walkthrough

Suppose the secret word is **`"apple"`** and the user guesses **`"peeps"`**:

* **Pass 1 (Green check & unmatched tally):**
  * Index `0`: `'a'` vs `'p'` -> unmatched; `remaining['a'] = 1`
  * Index `1`: `'p'` vs `'e'` -> unmatched; `remaining['p'] = 1`
  * Index `2`: `'p'` vs `'e'` -> unmatched; `remaining['p'] = 2`
  * Index `3`: `'l'` vs `'p'` -> unmatched; `remaining['l'] = 1`
  * Index `4`: `'e'` vs `'s'` -> unmatched; `remaining['e'] = 1`
  * *State after Pass 1:* `feedback = "BBBBB"`, `remaining = {'a': 1, 'p': 2, 'l': 1, 'e': 1}`

* **Pass 2 (Yellow check from left to right):**
  * Index `0` (`guess[0] = 'p'`): `remaining['p']` is `2 > 0` -> `feedback[0] = 'Y'`, `remaining['p']` becomes `1`.
  * Index `1` (`guess[1] = 'e'`): `remaining['e']` is `1 > 0` -> `feedback[1] = 'Y'`, `remaining['e']` becomes `0`.
  * Index `2` (`guess[2] = 'e'`): `remaining['e']` is now `0` -> stays `'B'`.
  * Index `3` (`guess[3] = 'p'`): `remaining['p']` is `1 > 0` -> `feedback[3] = 'Y'`, `remaining['p']` becomes `0`.
  * Index `4` (`guess[4] = 's'`): `remaining['s']` is `0` -> stays `'B'`.
  * *Final Result:* **`"YYBYB"`**.

Because `"apple"` produces `"YYBYB"` when evaluated against `"peeps"`, `"apple"` is preserved in the dictionary—whereas a naive global-ban set would have wrongly deleted `"apple"` because the second `'e'` was marked `'B'`.

### Single-Pass Dictionary Pruning (`potential_guesses`)

1. Validates that `target`, `input`, and `color` have length $L = 5$, `input` is alphabetic, and `color` contains only `'G'`, `'Y'`, or `'B'`.
2. Normalizes case and updates `target[i]` for every `'G'` tile.
3. Iterates through `dictionary` once using an active iterator:

```cpp
for (auto it = dictionary.begin(); it != dictionary.end(); ) {
    if (evaluate_guess(*it, input) != normalized_color) {
        it = dictionary.erase(it);
    } else {
        ++it;
    }
}
```

Using `it = dictionary.erase(it)` removes non-matching words in-place without allocating a temporary `toDelete` container or re-searching the tree.

## 3. Summary of Refactoring Improvements

| Area | Original Implementation | Refactored Implementation |
| :--- | :--- | :--- |
| **Duplicate `'B'` Tiles** | Globally banned any letter with a `'B'` tile, falsely eliminating valid secret words. | Two-pass count simulation handles duplicate `'G'`/`'Y'`/`'B'` combinations accurately. |
| **`'Y'` & `'G'` Counting** | Single candidate letter could satisfy multiple `'Y'` entries or overlap with a `'G'` slot. | Each letter in `candidate` is consumed at most once across Pass 1 and Pass 2. |
| **Heap Allocations** | Copied `std::set<std::pair<char, int>>` on the heap for every word in `dictionary`, plus built a `toDelete` set. | Zero heap allocations per word; uses a 256-int stack array and single-pass iterator erasure. |
| **Dictionary Loading** | Hardcoded ~2,000 `dictionary.insert(...)` calls inside `main()`. | Loaded dynamically from `data/WORDS` with CRLF trimming, deduplication, and validation. |
| **Headers & Standards** | Relied on non-standard `#include <bits/stdc++.h>` and `using namespace std;`. | Explicit standard headers, `wordle` namespace, and clean compilation under `-Wall -Wextra -Wpedantic -Werror`. |

## 4. Complexity Analysis

Let $N$ be the number of words in `dictionary`, $L$ be the word length ($L = 5$ in standard Wordle), and $K \le N$ be the number of surviving candidate words.

| Metric | Original | Refactored | Notes |
| :--- | :--- | :--- | :--- |
| **Per-Word Evaluation** | $O(L^2 \log L)$ | $O(L)$ | Two linear scans over length $L$ with $O(1)$ array lookups |
| **Dictionary Pruning** | $O(N \cdot L \log N)$ | $O(N \cdot L)$ | `dictionary.erase(it)` is amortized $O(1)$ per node |
| **Result Construction** | $O(K \cdot L)$ | $O(K \cdot L)$ | Copies the $K$ surviving words into the returned `std::vector` |
| **Total Time (General $L$)** | $O(N(L^2 \log L + L \log N))$ | $O(N \cdot L)$ | Scales linearly with dictionary size and word length |
| **Total Time ($L = 5$)** | $O(N \log N)$ | $O(N)$ | Strict linear time in dictionary size |
| **Auxiliary Space** | $O(N \cdot L)$ | $O(1)$ | Eliminates `toDelete` and per-word `std::set` copies |

## 5. Build, Test, and Usage

### Running Unit Tests

The test suite (`tests/test_wordle_filter.cpp`) executes 13 unit and integration tests covering file parsing, CRLF line endings, duplicate-letter edge cases, multi-turn games, case insensitivity, invalid inputs, and `data/WORDS` filtering.

* **Windows (MinGW):**
  ```powershell
  mingw32-make test
  ```
* **Linux / macOS:**
  ```bash
  make test
  ```

### Running the Interactive Solver

Start the CLI solver using `data/WORDS` (or pass a custom dictionary path as the first argument):

* **Windows (MinGW):**
  ```powershell
  mingw32-make run
  ```
* **Linux / macOS:**
  ```bash
  make run
  ```

Each turn, provide `<guess> <feedback>` separated by whitespace (or type `quit` to exit). The solver prints the count of remaining candidates followed by the matching words:

```text
adieu YBYBB
141
abbot ...
storm BBBBB
14
aback ...
quit
```

### Cleaning Build Artifacts

Remove compiled binaries and the `build/` directory before committing or archiving:

```powershell
mingw32-make clean   # or: make clean
```