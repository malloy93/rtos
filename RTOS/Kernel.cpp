#include "Kernel.hpp"
#include <algorithm>
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f429xx.h"
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"

// #include "Drivers/CMSIS/Include/cmsis_armcc.h"

#ifndef __ASM
#define __ASM __asm
#endif

extern core::RTCore* rtKernel;

extern "C" void start_thread_switch();
extern "C" void context_change();

namespace core
{

Thread* finishingThread;
Thread* startingThread;

void RTCore::init(utils::IdGen& idGen)
{
    kernelInstance = new RTCore(idGen);
}

RTCore* RTCore::getInstance()
{
    return kernelInstance;
}

void RTCore::createStack(uint16_t threadId)
{
    Stack* stack = new Stack{threadId};

    auto& threadStack = *stack;
    LOG_DEBUG("Init Stack: %d", threadId);
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
    LOG_DEBUG("Stack memory: ");
    for (int i = 0; i <= 17; i++)
    {
        LOG_DEBUG("%d: 0x%p: 0x%08X", i, &threadStack[stackSize - i], threadStack[stackSize - i]);
    }
    LOG_DEBUG("% 02X ", &threadStack[stackSize - 16]);

    threadControlBlocks[threadId]->setStackPtr((uint32_t)&threadStack);
    mappedStacks.push_back(stack);
}

uint16_t RTCore::createThread(void (*threadPointer)())
{
    auto threadId = idGen.getId();
    Thread* thread = new Thread{threadPointer, threadId};
    LOG_DEBUG("Thread created with ID: %d", threadId);
    thread->logLocalInfo();

    threadControlBlocks.push_back(thread);

    return threadId;
}

uint8_t RTCore::addThreads(std::vector<void (*)()>& threads)
{
    const auto numThreads = threads.size(); // 4
    LOG_INFO("Adding threads.");
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

void RTCore::initializeScheduler()
{
    for (const auto& thread : threadControlBlocks)
    {
        activeStacks.push_back(thread);
    }
    startingThread = getNextThread();
}

Thread* RTCore::getNextThreadRoundRobin()
{
    if (currentStackIndex >= activeStacks.size())
    {
        currentStackIndex = 0;
    }
    auto nextThread = activeStacks[currentStackIndex];
    currentStackIndex++;
    return nextThread;
}

void RTCore::logThreadInfo()
{
    for (const auto& thread : threadControlBlocks)
    {
        auto newBuffer = thread->printThreadInfo();
        LOG_DEBUG(newBuffer);
    }
}

void RTCore::launch(uint32_t quanta)
{
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    SysTick->LOAD = (quanta * prescaler) - 1;
    SYSPRI3 = (SYSPRI3 & 0x00FFFFFF) | 0xE0000000;

    SysTick->CTRL = 0x00000007;
    LOG_INFO("Launching scheduler");
    start_thread_switch();
}

void RTCore::remove(uint16_t threadId)
{
    LOG_INFO("Removing thread with ID: %d", threadId);
    auto activeit = std::remove_if(
        activeStacks.begin(),
        activeStacks.end(),
        [threadId](Thread* thread) { return thread->getThreadId() == threadId; });
    if (activeit != activeStacks.end())
    {
        activeStacks.erase(activeit, activeStacks.end());
        LOG_INFO("Thread with ID: %d removed successfully", threadId);
    }
    else
    {
        LOG_ERROR("Thread with ID: %d not found", threadId);
    }

    LOG_DEBUG("Removing stack for thread ID: %d", threadId);
    auto it = std::remove_if(
        mappedStacks.begin(), mappedStacks.end(), [threadId](Stack* stack) { return stack->getStackId() == threadId; });
    if (it != mappedStacks.end())
    {
        delete *it;
        mappedStacks.erase(it, mappedStacks.end());
        LOG_INFO("stack with ID: %d removed successfully", threadId);
    }
    else
    {
        LOG_ERROR("stack with ID: %d not found", threadId);
    }
}

void RTCore::suspend(uint16_t threadId)
{
    LOG_INFO("Suspending thread with ID: %d", threadId);
    auto it = std::remove_if(
        activeStacks.begin(),
        activeStacks.end(),
        [threadId](Thread* thread) { return thread->getThreadId() == threadId; });
    if (it != activeStacks.end())
    {
        activeStacks.erase(it, activeStacks.end());
        LOG_INFO("Thread with ID: %d suspended successfully", threadId);
    }
    else
    {
        LOG_ERROR("Thread with ID: %d not found", threadId);
    }
}

uint16_t RTCore::add(void (*threadPointer)())
{
    auto threadId = createThread(threadPointer);
    createStack(threadId);
    activeStacks.push_back(threadControlBlocks.back());
    LOG_INFO("Thread with ID: %d added successfully", threadId);
    return threadId;
}
} // namespace core

extern "C" void changeContext()
{
    core::finishingThread = core::startingThread;
    core::startingThread = core::RTCore::getInstance()->getNextThread();
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

// extern "C" void PendSV_Handler()
// {
//     context_change();
// }

// sep - oct 2025 - SVC + refactor
// nov - scheduler + stack + refactor
// dec - jan - mutex + semaphores + queues
// feb - mar - sys reconfig

extern "C" __attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile(
        // Wybór stack pointera: PSR[2] w EXC_RETURN (LR)
        "tst lr, #4           \n" // bit2: 0->MSP, 1->PSP
        "ite eq               \n"
        "mrseq r0, msp        \n"
        "mrsne r0, psp        \n"
        // Sprawdź obecność ramki FPU (bit4 EXC_RETURN):
        // bit4 == 0 => rozszerzona ramka (FPU) znajduje się NA STOSIE przed standardową,
        // trzeba ją ominąć +18 słów (s0-s15,FPSCR,RESERVED) = 18*4 = 72 bajty.
        "tst lr, #16          \n" // bit4: 1 = brak ramki FPU, 0 = jest ramka FPU
        "it eq                \n"
        "addeq r0, r0, #72    \n" // pomiń rozszerzoną ramkę
        "b SVC_Handler_C      \n");
}

extern "C" void SVC_Handler_C(uint32_t* stacked)
{
    uint32_t pc = stacked[6];
    uint8_t svc_imm = *reinterpret_cast<uint8_t*>(pc - 2);

    core::RTCore* core = core::RTCore::getInstance();

    switch (svc_imm)
    {
        case core::SVC_ADDTASK:
        {
            core::taskP cb = reinterpret_cast<core::taskP>(stacked[0]);
            core->add(cb);
            break;
        }
        case core::SVC_REMOVETASK:
        {
            uint16_t threadId = static_cast<uint16_t>(stacked[0]);
            core->remove(threadId);
            break;
        }
        case core::SVC_SUSPENDASK:
        {
            uint16_t threadId = static_cast<uint16_t>(stacked[0]);
            core->suspend(threadId);
            break;
        }

        default:
            // Nieznany SVC
            break;
    }
}
