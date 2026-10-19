.syntax unified
.cpu cortex-m4
.thumb

.global Reset_Handler
.global Vector_Table

/* Definiciones de símbolos externos del Linker Script */
.word _estack
.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

.section .isr_vector,"a",%progbits
.type Vector_Table, %object
Vector_Table:
    .word _estack               /* Initial Stack Pointer */
    .word Reset_Handler         /* Reset Handler */
    .space 400                  /* Espacio para handlers de interrupción */

.section .text.Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    /* 1. Copiar sección .data desde Flash a RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata

copy_data_loop:
    cmp r0, r1
    bhs copy_data_done
    ldr r3, [r2], #4
    str r3, [r0], #4
    b copy_data_loop

copy_data_done:
    /* 2. Limpiar sección .bss en RAM a 0 */
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0

zero_bss_loop:
    cmp r0, r1
    bhs zero_bss_done
    str r2, [r0], #4
    b zero_bss_loop

zero_bss_done:
    /* 3. Llamar a main() */
    bl main

default_loop:
    b default_loop
