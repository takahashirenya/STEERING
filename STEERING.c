#include <stdio.h>
#include "pico/stdlib.h"
#include "servo.h"


int main()
{
    stdio_init_all();
    tail_setup();
    while (true)
    {
        tail_controller();
        sleep_ms(20);
    }
}
