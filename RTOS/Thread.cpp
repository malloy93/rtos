#include "Thread.hpp"
#include "Logger.hpp"

namespace kernel
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
    logger->logDebug(newBuffer);
    // logger->logDebug(oss.str());
}

void Thread::logSizeChange()
{
    // if (threadId == 0)
    // {
    auto remainingStackSize = (uint32_t)stackPtr - startPtr;
    logger->logDebug("Thread: ", threadId);
    // logger->logDebug("Start stack size: ", startPtr);
    // logger->logDebug("Current stack size: ", (uint32_t)stackPtr);
    logger->logDebug("Used stack size: ", remainingStackSize);
    // }
}

} // namespace kernel