#include  <stdint.h>
int main(void) {
    /* Bucle principal simple de prueba */
    volatile uint32_t counter = 0;

    while (1) {
        counter ++ ;
    }
    return 0;
}