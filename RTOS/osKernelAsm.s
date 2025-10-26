.section .text
.cpu cortex-m4
.syntax unified
.global _ZN6kernel10currentPtrE
.global SysTick_Handler
.global PendSV_Handler
.global changeContext
.thumb

.type PendSV_Handler, function
.word PendSV_Handler
.type SysTick_Handler, function
.word SysTick_Handler

.global start_thread_switch
.global context_change

start_thread_switch:
    LDR R0, =_ZN6kernel14startingThreadE  // Załaduj adres globalnego wskaźnika do R0
    LDR R0, [R0]                      // Załaduj wartość globalnego wskaźnika (adres pierwszego wątku) do R0
    LDR R0, [R0]                      // Przesuń R0 na adres stackPtr w strukturze Thread (pierwsze pole)
    MSR PSP, R0                       // Ustaw PSP na adres stosu pierwszego wątku
    MOV R0, #2                        // Ustawienie CONTROL na korzystanie z PSP i trybu uprzywilejowanego
    MSR CONTROL, R0                   // Zapisz kontrolny rejestr
    POP {R4-R11}                      // Przywróć rejestry kontekstu z PSP (R4-R11)
    POP {R0-R3}                       // Przywróć rejestry R0-R3 z PSP
    POP {R12}                         // Przywróć rejestr R12 z PSP
    ADD SP, SP, #4                    // Korekta stosu dla przesunięcia PC
    POP {LR}                          // Przywróć Link Register z PSP
    ADD SP, SP, #4                    // Korekta stosu dla przesunięcia xPSR
    CPSIE I                           // Włącz przerwania
    BX LR                             // Skok do adresu zawartego w LR

PendSV_Handler:
    CPSID I                      

    LDR R0, =thread_switch_counter
    LDR R1, [R0]
    ADD R1, R1, #1
    STR R1, [R0]

    // Store current context
    MRS R0, PSP                  // Skopiuj aktualną wartość PSP do R0 (PSP wskazuje na stos bieżącego wątku)
    STMDB R0!, {R4-R11}          // Zapisz rejestry R4-R11 na stosie bieżącego wątku
    LDR R1, =_ZN6kernel15finishingThreadE	  // Załaduj adres wskaźnika na kończący się wątek
    LDR R2, [R1]                 // Załaduj wskaźnik na kończący się wątek
    STR R0, [R2]                 // Zaktualizuj wskaźnik stosu w kończącym się wątku (zapisz PSP bieżącego wątku)

    // Switch to new thread
    LDR R0, =_ZN6kernel14startingThreadE     // Załaduj adres wskaźnika na nowy wątek
    LDR R0, [R0]                 // Pobierz wskaźnik na nowy wątek
    LDR R0, [R0]                 // Załaduj wskaźnik stosu nowego wątku (PSP nowego wątku)
    MSR PSP, R0                  // Ustaw PSP na stos nowego wątku

    // Restore context of new thread
    LDMIA R0!, {R4-R11}          // Przywróć rejestry R4-R11 ze stosu nowego wątku
    MSR PSP, R0                  // Aktualizuj PSP do nowego stosu (po przywróceniu rejestrów)

    CPSIE I                      // Włącz przerwania
    BX LR                        // Powrót z przerwania


SysTick_Handler: 
            CPSID   I       
            PUSH {LR}
            BL      changeContext     
            POP {LR}
            CPSIE   I                  
            BX		  LR


