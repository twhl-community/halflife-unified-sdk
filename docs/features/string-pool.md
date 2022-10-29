# String Pool

The Half-Life Unified SDK uses a string pool implementation that eliminates the use of the engine's internal heap for strings.

The engine's `pfnAllocString` function is used in the original SDK. This function allocates memory from the engine heap every time it's called.
Even if a string has already been allocated it will be allocated again which means if there is any entity setup or logic in player frame code using the `ALLOC_STRING` macro there is a linear increase in memory usage over time.

The engine's heap will eventually run out of memory, causing a fatal error with the message `Hunk_Alloc: failed on <size> bytes` where `<size>` is the size of the string being allocated.

The new string pool is detached from the engine's heap and pools its strings, so repeated calls with the same string won't allocate more memory. In most cases the string pool's memory usage should level off after map spawn (when most strings are allocated for the first time).

This reduces the pressure on the engine's heap and allows strings to be allocated without resulting in a linear increase in memory usage.

The pool is cleared at the start of a map. Since strings are pooled all `string_t` strings are read-only, which was implicitly the case before but is now required to avoid problems where modifying a single instance modifies all uses of the same string.

Additionally, the engine's version also (poorly) handles escape sequences. Any occurrence of `\` followed by `n` is converted to just `\n` (newline character), while any occurrence of `\` followed by any other character is turned into just `\`, with the character that follows being removed.

In most cases this behavior is unwanted and causes subtle bugs, for instance when selecting paths in a level editor if the path contains a `\` the path will be modified on allocation, causing strange bugs. Since `\` is the default path separator on Windows this can happen often.

The recommended path separator is `/`. The engine's filesystem internally converts paths to the default path separator so this will rarely require manual replacement.

In the rare case where this behavior is wanted a separate function called `ALLOC_ESCAPED_STRING` will perform this conversion. It is currently used only in `game_text`'s `message` keyvalue to handle newlines.
