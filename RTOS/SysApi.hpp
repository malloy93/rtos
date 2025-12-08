#pragma once

namespace core
{
using taskP = void (*)(void);

static inline void addTask(taskP taskPointer)
{
    register taskP r0 __asm("r0") = taskPointer; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_ADDTASK)
                   : "memory");
}
static inline void removeTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_REMOVETASK)
                   : "memory");
}

static inline void suspendTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_SUSPENDASK)
                   : "memory");
}
static inline void resumeTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_RESUMETASK)
                   : "memory");
}
} // namespace core
