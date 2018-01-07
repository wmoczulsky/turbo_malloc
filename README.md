

# Requirements:
- chunk must be released if is empty and empty space in other chunks is above specified treshold
- if user requested block larger than several pages, then whole chunk should be given
- first-fit
- double linked list of blocks sorted by address
- eager coalescence 
- thread safety
- mdump 

# What I've Done:
- big block allocator - allocates whole chunk for block
- abstract allocator class and virtual dispatching. $ operator means calling virtual method.
- Canaries. Every important data structure has constant value named "canary", which is sometimes checked in order to detect data corruption.




Because of some assumpions and numerous undefined behaviour, please use gcc version 5.4.0 in case of problems.