#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include "Logger.hpp"
#include "Stack.hpp"
#include "Thread.hpp"
#include "Utils.hpp"

#define SYSPRI3 (*((volatile uint32_t*)0xE000ED20))
#define INTCTRL (*((volatile uint32_t*)0xE000ED04))

namespace kernel
{

constexpr int stackSize{500};

using threadId = uint8_t;

class Wrapper
{
};

class Kernel
{
public:
    Kernel(Logger& logger, utils::IdGen& idGen) : logger{logger}, idGen{idGen}
    {
        logger.logInfo("Initializing kernel");
        prescaler = utils::getClockFreq() / 1000;
    }

    uint8_t addThreads(std::vector<void (*)()>&);
    void kernelLaunch(uint32_t);

    void logThreadInfo();

    // Implementation
    void removeThread(threadId);
    void addThread(Wrapper&);
    void addThread(void (*)());

    void add();

private:
    uint16_t createThread(void (*)());
    void initializeStack(uint16_t);

    std::vector<void (*)()> pendingTaskList;

    Logger& logger;
    utils::IdGen& idGen;

    uint32_t prescaler{};
    std::vector<Thread*> threadControlBlocks; // change to unordered_map [threadId, tcb] or not // no this is dumb

    std::map<uint16_t, Stack*> stackPointers;
};

} // namespace kernel

extern "C" void changeContext();

/// New rtos development plan
// 1.1. ReUse previous code - done
// 1.2. Remove std::string from logger - done
// 2.0 PSP support - working
// 2.1. Refactor existing code - ongoing sept 24
// 2.2. Add interrupt handling - partially done sept 24
// 2.3. Switch logger to nonblocking - postponed oct 24
// 3. Single thread addition - planned sept 24
// 4. Single thread removal - planned sept 24
// 5. Self-reconfig task/thread - planned oct 24
// 6. Memory management - planned oct 24
// 7. Mutexes and semaphores - planned oct 24
// 8. Clean-up task - planned nov 24
// 9. SVC support - planned nov 24
// 10. Interrupts - planned nov 24
// 11. system restart 3 levels - planned dec 24
// 12. console control - planned dec 24
// 13. external memory for logging and data - planned dec 24
// 14. RTC - planned jan 25
// 15. config file - planned jan 25
// 16. Tasks with priorities - planned jan 25 (maybe earlier)
// 17. Error handling - planned feb 25