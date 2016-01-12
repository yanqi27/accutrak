
AccuTrak

A configurable memory debug tool. It detects general memory errors, such as overrun, underrun, double delete, invalid access of freed memory as well as memory leaks.

The tool works in two major modes: hardware padding and software padding. (1) The hardware padding uses a system page to protect user memory space. The system page is placed immediately in front or after the user space and is set to not accessible. If memory overrun or underrun happens due to user code error, a hardware exception occurs which points the user to the exact location of offending code. After memory is freed, both user space and system page are set to not accessible. Hence, any access of freed memory will cause a hardware exception and user can easily pinpoint the bug. (2) The software padding is somewhat similar. But instead of using inaccessible system page, it pads a few bytes around the user space. It is significantly faster than hardware padding mode but less effective because the tool can only check the integrity of the memory blocks at the entry points of the tool.

One of the major obstacles of similar tools is the performance degradation due to excessive system calls to manage the protecting pages. The slowdown could range from 10 times slower to 100 times or even 1000 times slower. Hence, it is not feasible to use this kind of tool in many applications. The solution used by AccuTrak is to only track modules that interest us. System libraries and well-tested trusted modules are not the concern when we debug our program. This is especially true when we are in feature development phase. Untracked modules use default memory allocator to allocate/release memory blocks. Special cares are taken to handle the memory blocks belonging either to default allocator or to AccuTrak so that they coexist seamlessly. This makes the program run significantly faster and the tool useful.

The tool is designed to work on most popular platforms using virtual memory. It is coded to achieve maximum speed and least space overhead in most cases. It is thread-safe.

AccuTrak uses a configuration file to set the behavior of the tool. User doesn't have to  recompile or relink to use different features of the tool. For example to switch from hardware mode to software mode, or add/subtract modules from tracking list. The tool can be preloaded into the user program or linked in executable if preloading is not supported by OS.

