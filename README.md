# Summary:
I have implemented malloc with 3 different allocation algorithms:
- big-block - used for the bigest allocations, just gives whole pages
- bitmap allocator - which uses the least amount of memory while allocating small blocks
- traditional first-fit 

# Also:
- good abstraction of allocators, virtual methods, and abstract class
- canaries in order to detect internal data structures corruptions
- one really good functional test
- file structure, that moves platform-specific code in one plece
- millions of asserts, which helps to quickly diagnose problems, and wrong input (non-existing ptr as free() argument)
- modest mdump()

# What is not implemented:
- unit tests - I used this functional test and asserts during development, and adding unit tests at the end is unnatural
