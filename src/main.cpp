
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <intrin.h>
#include <iostream>
#include <string>
#include <thread>

// NOTE: This is a macro so that we don't have to constantly deal with type cast warnings
#define WORD_LENGTH 5

struct dictionary_entry
{
    unsigned char letter_indices[WORD_LENGTH];
    std::uint32_t letter_masks[WORD_LENGTH];
    unsigned char letter_counts[26] = {};

    dictionary_entry(std::string_view str)
    {
        assert(str.size() == WORD_LENGTH);
        for (std::size_t i = 0; i < WORD_LENGTH; ++i)
        {
            auto ch = str[i];
            assert((ch >= 'A') && (ch <= 'Z'));
            letter_indices[i] = ch - 'A';
            letter_masks[i] = 0x01 << letter_indices[i];
            ++letter_counts[letter_indices[i]];
        }
    }
};

enum class letter_result
{
    incorrect = 0,
    incorrect_location = 1,
    correct = 2,
};

struct game_state
{
    std::uint32_t letter_masks[WORD_LENGTH];
    std::pair<int, int> letter_counts[26];

    game_state()
    {
        for (auto& mask : letter_masks) mask = (0x01 << 26) - 1;
        for (auto& pair : letter_counts) pair = { 0, WORD_LENGTH };
    }
};

std::vector<dictionary_entry> dictionary;
std::size_t dictionary_size; // We don't remove things from the dictionary vector, hence the second length

static std::filesystem::path find_dictionary_path()
{
    auto path = std::filesystem::current_path();
    while (true)
    {
        auto testPath = path / L"dictionary.txt";
        if (std::filesystem::exists(testPath))
        {
            return testPath;
        }

        auto temp = path.parent_path();
        if (temp == path)
        {
            return path;
        }

        path.swap(temp);
    }
}

static bool word_fits_requirements(const dictionary_entry& entry, const game_state& state)
{
    for (std::size_t i = 0; i < WORD_LENGTH; ++i)
    {
        if (!(entry.letter_masks[i] & state.letter_masks[i])) return false;
    }

    for (std::size_t i = 0; i < 26; ++i)
    {
        if (entry.letter_counts[i] < state.letter_counts[i].first) return false;
        if (entry.letter_counts[i] > state.letter_counts[i].second) return false;
    }

    return true;
}

static std::size_t prune_dictionary(const game_state& state, bool actuallyRemove = true)
{
    std::size_t result = 0;
    for (std::size_t i = 0; i < dictionary_size; )
    {
        if (!word_fits_requirements(dictionary[i], state))
        {
            ++result;
            if (actuallyRemove)
            {
                std::swap(dictionary[i], dictionary[dictionary_size - 1]);
                --dictionary_size;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            ++i;
        }
    }

    return result;
}

static game_state apply(game_state state, const dictionary_entry& entry,
    const std::array<letter_result, WORD_LENGTH>& feedback)
{
    int letterCounts[26] = {};
    bool reachedMaxLetters[26] = {};
    for (std::size_t i = 0; i < WORD_LENGTH; ++i)
    {
        auto letterIndex = entry.letter_indices[i];
        auto letterMask = entry.letter_masks[i];
        switch (feedback[i])
        {
        case letter_result::incorrect:
            state.letter_masks[i] &= ~letterMask;
            reachedMaxLetters[letterIndex] = true;
            break;

        case letter_result::incorrect_location:
            state.letter_masks[i] &= ~letterMask;
            ++letterCounts[letterIndex];
            break;

        case letter_result::correct:
            state.letter_masks[i] = letterMask;
            ++letterCounts[letterIndex];
            break;
        }

        if (reachedMaxLetters[letterIndex])
        {
            state.letter_counts[letterIndex].second = letterCounts[letterIndex];
        }
        if (letterCounts[letterIndex] > state.letter_counts[letterIndex].first)
        {
            state.letter_counts[letterIndex].first = letterCounts[letterIndex];
        }
    }

    return state;
}

static std::size_t word_min_removed(const game_state& state, const dictionary_entry& entry)
{
    std::size_t result = std::numeric_limits<std::size_t>::max();
    std::array<letter_result, WORD_LENGTH> results;
    for (int a0 = 0; a0 < 3; ++a0)
    {
        results[0] = static_cast<letter_result>(a0);
        for (int a1 = 0; a1 < 3; ++a1)
        {
            results[1] = static_cast<letter_result>(a1);
            for (int a2 = 0; a2 < 3; ++a2)
            {
                results[2] = static_cast<letter_result>(a2);
                for (int a3 = 0; a3 < 3; ++a3)
                {
                    results[3] = static_cast<letter_result>(a3);
                    for (int a4 = 0; a4 < 3; ++a4)
                    {
                        results[4] = static_cast<letter_result>(a4);
                        auto tempState = apply(state, entry, results);
                        auto wouldRemove = prune_dictionary(tempState, false);
                        if (wouldRemove < result)
                        {
                            result = wouldRemove;
                        }
                    }
                }
            }
        }
    }

    return result;
}

static std::size_t select_word(game_state& state)
{
    // If this is the first iteration, we already know the best word, so optimize
    // if (state.letter_masks[0] == ((0x01 << 26) - 1))
    // {
    //     auto itr = std::find_if(dictionary.begin(), dictionary.end(), [&](const auto& entry) {
    //         return entry.letter_masks[0] == (0x01 << ('S' - 'A')) &&
    //             entry.letter_masks[1] == (0x01 << ('E' - 'A')) &&
    //             entry.letter_masks[2] == (0x01 << ('R' - 'A')) &&
    //             entry.letter_masks[3] == (0x01 << ('A' - 'A')) &&
    //             entry.letter_masks[4] == (0x01 << ('I' - 'A'));
    //     });
    //     return itr - dictionary.begin();
    // }

    // Check every word in the dictionary to see which one is guaranteed to reduce the set down the most
    auto threadCount = std::thread::hardware_concurrency();
    auto wordsPerThread = (dictionary.size() + threadCount - 1) / threadCount;
    std::vector<std::pair<std::size_t, std::size_t>> results(threadCount);
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < threadCount; ++i)
    {
        auto start = wordsPerThread * i;
        auto end = std::min(start + wordsPerThread, dictionary.size());
        threads.emplace_back([i, start, end, &state, &results]() mutable {
            std::size_t result = 0;
            std::size_t mostRemoved = 0;
            for (; start < end; ++start)
            {
                auto removed = word_min_removed(state, dictionary[start]);
                if (removed > mostRemoved)
                {
                    result = start;
                    mostRemoved = removed;
                }
            }

            results[i] = { result, mostRemoved };
        });
    }

    for (auto& thread : threads) thread.join();

    std::size_t result = 0;
    std::size_t mostRemoved = 0;
    for (auto& pair : results)
    {
        if (pair.second > mostRemoved)
        {
            result = pair.first;
            mostRemoved = pair.second;
        }
    }

    std::printf("This word is guaranteed to reduce dictionary size by at least %zu\n", mostRemoved);

    return result;
}

static void print_word(const dictionary_entry& entry)
{
    for (auto index : entry.letter_indices)
    {
        std::printf("%c", 'A' + index);
    }
}

int main()
{
    auto dictPath = find_dictionary_path();
    if (!dictPath.has_filename())
    {
        std::printf("ERROR: Unable to find dictionary file. Ensure that the working directory is set correctly\n");
        return 1;
    }

    std::ifstream dictStream(dictPath);
    assert(dictStream); // We found the path, should be able to open

    dictionary.reserve(12972); // From dictionary file
    std::string line;
    while (std::getline(dictStream, line))
    {
        if (line.size() == WORD_LENGTH)
        {
            dictionary.emplace_back(line);
        }
    }
    dictionary_size = dictionary.size();

    std::printf(R"^-^(
Welcome to the wordle solver! Each round you will be presented with a word to submit and then provide feedback for the
results. The key is below:

    O       The letter is present in the final word and is in the correct location
    -       The letter is present in the final word, but is not in the correct location
    X       The letter is not present in the final word

There are also a few additional commands you can execute:

    list    This will list all remaining words in the dictionary

)^-^");

    game_state state;
    for (bool done = false; !done; )
    {
        std::printf("There are %zu words left in the dictionary\n", dictionary_size);
        auto index = select_word(state);
        std::printf("Submit this word: ");
        print_word(dictionary[index]);
        std::printf("\n");

        while (true)
        {
            std::printf("Result:           ");
            if (!std::getline(std::cin, line)) return 1;
            for (auto& ch : line)
            {
                if ((ch >= 'a') && (ch <= 'z')) ch = ch - 'a' + 'A';
            }

            if (line == "LIST")
            {
                std::printf("The remaining words in the dictionary are: ");
                const char* prefix = "";
                for (std::size_t i = 0; i < dictionary_size; ++i)
                {
                    std::printf("%s", prefix);
                    prefix = ", ";
                    print_word(dictionary[i]);
                }
                std::printf("\n");

                continue;
            }
            else if (line.size() != WORD_LENGTH)
            {
                std::printf("ERROR: Input was of incorrect length. Please only use the letters 'X', '-', and 'O'\n");
                continue;
            }

            std::array<letter_result, WORD_LENGTH> feedback;
            bool badInput = false;
            bool allCorrect = true;
            for (std::size_t i = 0; (i < WORD_LENGTH) && !badInput; ++i)
            {
                switch (line[i])
                {
                case 'X':
                    feedback[i] = letter_result::incorrect;
                    allCorrect = false;
                    break;

                case '-':
                    feedback[i] = letter_result::incorrect_location;
                    allCorrect = false;
                    break;

                case 'O':
                    feedback[i] = letter_result::correct;
                    break;

                default:
                    std::printf(
                        "ERROR: Input contained invalid character(s). Please only use the letters 'X', '-', and 'O'\n");
                    badInput = true;
                    break;
                }
            }

            if (badInput) continue;
            done = allCorrect;

            state = apply(state, dictionary[index], feedback);
            prune_dictionary(state);
            break;
        }
    }
}
