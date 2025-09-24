#include "Kernel.hpp"
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f429xx.h"
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"

// #include "Drivers/CMSIS/Include/cmsis_armcc.h"

#ifndef __ASM
#define __ASM __asm
#endif

extern kernel::Kernel* rtKernel;

extern "C" void start_thread_switch();

namespace kernel
{

Thread* finishingThread;
Thread* startingThread;

void Kernel::createStack(uint16_t threadId)
{
    Stack* stack = new Stack{logger, threadId};

    auto& threadStack = *stack;
    logger.logDebug("Init Stack: %d", threadId);
    threadControlBlocks[threadId]->stackPtr = &threadStack[stackSize - 16];
    threadStack[stackSize - 1] = 0x01000000; // thumb xPSR
    threadStack[stackSize - 2] = (uint32_t)(threadControlBlocks[threadId]->threadPointer);
    threadStack[stackSize - 3] = 0xFFFFFFFD; // LR to EXC_RETURN in Thread Mode
    threadStack[stackSize - 4] = 0x12121212; // R12
    threadStack[stackSize - 5] = 0x03030303; // R3
    threadStack[stackSize - 6] = 0x02020202; // R2
    threadStack[stackSize - 7] = 0x01010101; // R1
    threadStack[stackSize - 8] = (uint32_t)0; // R0

    // R4 - R11 - dummy values
    for (int i = 9; i <= 16; i++)
    {
        threadStack[stackSize - i] = 0x04040404;
    }
    logger.logDebug("Stack memory: ");
    for (int i = 0; i <= 17; i++)
    {
        logger.logDebug(" % 02X ", threadStack[stackSize - i]);
    }
    logger.logDebug("% 02X ", &threadStack[stackSize - 16]);

    threadControlBlocks[threadId]->setStackPtr((uint32_t)&threadStack);
}

uint16_t Kernel::createThread(void (*threadPointer)())
{
    auto threadId = idGen.getId();
    Thread* thread = new Thread{threadPointer, threadId, &logger};
    logger.logDebug("Thread created with ID: %d", threadId);
    thread->logLocalInfo();

    threadControlBlocks.push_back(thread);

    return threadId;
}

uint8_t Kernel::addThreads(std::vector<void (*)()>& threads)
{
    const auto numThreads = threads.size(); // 4
    logger.logInfo("Adding threads.");
    for (std::size_t i = 0; i < numThreads; i++)
    {
        createThread(threads.at(i));
    }

    for (std::size_t i = 0; i < numThreads; i++)
    {
        auto threadId = threadControlBlocks[i]->getThreadId();
        createStack(threadId);
    }
    initializeScheduler();

    logThreadInfo();
    return 1;
}

void Kernel::initializeScheduler()
{
    for (const auto& thread : threadControlBlocks)
    {
        activeStacks.push_back(thread);
    }
    startingThread = getNextThread();
}

Thread* Kernel::getNextThreadRoundRobin()
{
    if (currentStackIndex >= activeStacks.size())
    {
        currentStackIndex = 0;
    }
    auto nextThread = activeStacks[currentStackIndex];
    currentStackIndex++;
    return nextThread;
}

void Kernel::logThreadInfo()
{
    for (const auto& thread : threadControlBlocks)
    {
        auto newBuffer = thread->printThreadInfo();
        logger.logDebug(newBuffer);
    }
}

void Kernel::launch(uint32_t quanta)
{
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    SysTick->LOAD = (quanta * prescaler) - 1;
    SYSPRI3 = (SYSPRI3 & 0x00FFFFFF) | 0xE0000000;

    SysTick->CTRL = 0x00000007;
    logger.logInfo("Launching scheduler");
    start_thread_switch();
}

} // namespace kernel

extern "C" void changeContext()
{
    kernel::finishingThread = kernel::startingThread;
    // kernel::startingThread = kernel::finishingThread->getNextPtr();
    kernel::startingThread = rtKernel->getNextThread();
    INTCTRL = 0x10000000;
}

// extern "C"
// {
//     void SysTick_Handler()
//     {
//         __asm("CPSID I");
//         changeContext();
//         __asm("CPSIE I");
//     }
// }