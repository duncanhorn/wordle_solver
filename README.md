# Wordle Solver
Solves a [wordle](https://www.powerlanguage.co.uk/wordle/) puzzle such that each guess is optimized to maximize the minimum number of remaining words that get eliminated from consideration.
This approach should guarantee the fewest average number of guesses to arrive at the final word, which generally requires 4-5 guesses, with it occasionally arriving at the final solution in fewer guesses.

The dictionary used is the same as the dictionary used by the website.
Note that the website effectively has two dictionaries: one that contains all past and future "words of the day" that you try and guess, and another that includes all other words that are considered valid guesses.
By default, this repo uses the union of these two sets of words, even though this means that possible guesses may never appear as the final word - at least not in the forseeable future.
This approach seemed the most fair as it's assumed that the final word can be any valid 5 letter word.
A separate dictionary is also provided that only contains the much smaller set of valid answers.
This dictionary can be accessed by running `wordle.exe --small` from the command line.
Using the smaller dictionary typically reduces the number of required guesses down to about 3-4.
