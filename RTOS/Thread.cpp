#include "Thread.hpp"
#include "Logger.hpp"

namespace core
{
std::ostream& operator<<(std::ostream& os, const Thread& thread)
{
    os << "Thread Info: ";
    os << " stackPtr: " << thread.stackPtr;
    os << " ,nextPtr: " << thread.nextPtr;
    // os << " ,threadPointer: " << reinterpret_cast<void*>(thread.threadPointer);
    os << " ,threadId: " << thread.getThreadId();

    // Prywatne pole "priority" można wypisać tylko, jeśli dodasz getter w klasie
    // os << "priority: " << thread.getPriority() << std::endl;

    return os;
}

const char* Thread::printThreadInfo()
{
    static char buffer[256];
    snprintf(
        buffer, sizeof(buffer), "Thread Info: stackPtr: %p ,nextPtr: %p ,threadId: %d", stackPtr, nextPtr, threadId);
    return buffer;
}

void Thread::logLocalInfo()
{
    auto newBuffer = printThreadInfo();
    LOG_DEBUG("%s", newBuffer);
    // LOG_DEBUG(oss.str());
}

void Thread::logSizeChange()
{
    // if (threadId == 0)
    // {
    auto remainingStackSize = reinterpret_cast<uintptr_t>(stackPtr) - static_cast<uintptr_t>(startPtr);
    LOG_DEBUG("Thread: %d", threadId);
    // LOG_DEBUG("Start stack size: ", startPtr);
    // LOG_DEBUG("Current stack size: ", (uint32_t)stackPtr);
    LOG_DEBUG("Used stack size: %lu", static_cast<unsigned long>(remainingStackSize));
    // }
}

} // namespace core
