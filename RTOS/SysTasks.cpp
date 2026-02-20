#include "SysTasks.hpp"

#include "Core.hpp"
#include "Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
#include "Logger.hpp"
namespace sysTasks
{

void idleTask()
{
    while (true)
    {
        __WFI(); // Wait For Interrupt
    }
}

void systemReconfigurationTask()
{
    while (true)
    {
        __WFI(); // Wait For Interrupt
    }
}

void eventTracerTask()
{
    core::Logger* logger = core::Logger::getInstance();
    while (true)
    {
        logger->send();
        __WFI(); // Wait For Interrupt
    }
}

void resourceAllocatorTask()
{
    core::RTCore* core = core::RTCore::getInstance();

    while (true)
    {
        core->runAllocator();
        __WFI(); // Wait For Interrupt
    }
}
void resourceUpdateTask()
{
    while (true)
    {
        __WFI(); // Wait For Interrupt
    }
}

} // namespace sysTasks