.section .text, "ax", %progbits
.cpu cortex-m4
.syntax unified
.thumb

.extern _ZN4core14startingThreadE
.extern _ZN4core15finishingThreadE
.extern thread_switch_counter
.extern changeContext


.align 2
.global start_thread_switch
.type start_thread_switch, %function
.thumb_func
start_thread_switch:
    LDR     R0, =_ZN4core14startingThreadE
    LDR     R0, [R0]
    LDR     R0, [R0]

    MSR     PSP, R0

    MOV     R0, #2
    MSR     CONTROL, R0
    ISB

    POP     {R4-R11}
    POP     {R0-R3}
    POP     {R12}

    ADD     SP, SP, #4
    POP     {LR}
    ADD     SP, SP, #4

    CPSIE   I
    BX      LR

.size start_thread_switch, .-start_thread_switch


.align 2
.global PendSV_Handler
.type PendSV_Handler, %function
.thumb_func
PendSV_Handler:
    CPSID   I

    LDR     R0, =thread_switch_counter
    LDR     R1, [R0]
    ADD     R1, R1, #1
    STR     R1, [R0]

    // Zachowanie kontekstu bieżącego wątku
    MRS     R0, PSP
    STMDB   R0!, {R4-R11}

    LDR     R1, =_ZN4core15finishingThreadE
    LDR     R2, [R1]
    STR     R0, [R2]

    // Przełączenie na stos następnego wątku
    LDR     R0, =_ZN4core14startingThreadE
    LDR     R0, [R0]
    LDR     R0, [R0]
    MSR     PSP, R0

    // Odtworzenie kontekstu następnego wątku
    LDMIA   R0!, {R4-R11}
    MSR     PSP, R0

    CPSIE   I
    BX      LR

.size PendSV_Handler, .-PendSV_Handler


.align 2
.global SysTick_Handler
.type SysTick_Handler, %function
.thumb_func
SysTick_Handler:
    CPSID   I
    PUSH    {LR}

    BL      changeContext

    POP     {LR}
    CPSIE   I
    BX      LR

.size SysTick_Handler, .-SysTick_Handler