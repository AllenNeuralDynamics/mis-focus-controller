#ifndef CORE_1_MAIN
#define CORE_1_MAIN

#include <config.h>
#include <stdint.h>
#include <pico/stdlib.h>

// Double buffer ptr for sharing access to the encoder data across cores.
extern uint32_t* read_buffer_ptr;

// function prototypes
void update_encoders();
void init_encoder_pins();
void core1_main();


#endif // CORE_1_MAIN