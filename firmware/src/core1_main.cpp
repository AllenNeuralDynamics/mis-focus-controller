#include <core1_main.h>

#ifdef PROFILE_ENCODER_LOOP
#define SYST_CSR (*(volatile uint32_t*)(PPB_BASE + 0xe010))
#define SYST_CVR (*(volatile uint32_t*)(PPB_BASE + 0xe018))
#endif

// Double buffer for writing encoder data (Core1) and reading it (Core0).
int32_t encoder_buffers[2][NUM_BMCS];
int32_t* write_buffer_ptr = encoder_buffers[0];
int32_t* read_buffer_ptr = encoder_buffers[1];

// raw gpio value from the previous read.
uint32_t prev_state;
uint32_t curr_state;


void init_encoder_pins()
{
    // Encoder GPIOs are contiguous starting from ENCODER_BASE_OFFSET
    for (auto i=0; i<NUM_BMCS-1; ++i)
    {
        // Setup GPIOs as inputs. (No pullups required.)
        gpio_init(ENCODER_BASE_OFFSET + 2*i);
        gpio_init(ENCODER_BASE_OFFSET + 2*i + 1);
    }
    // Handle final encoder case.
    uint32_t LAST_ENC_BASE = 26;
    gpio_init(LAST_ENC_BASE);
    gpio_init(LAST_ENC_BASE + 1);
}


int32_t get_encoder_increment(uint32_t prev_state, uint32_t curr_state)
{
    switch ( uint8_t( (prev_state << 2) | curr_state) )
    {
        // forward cases:
        case 0b0001:
        case 0b0111:
        case 0b1110:
        case 0b1000:
            return 1;
        // backward cases:
        case 0b0010:
        case 0b1011:
        case 0b1101:
        case 0b0100:
            return -1;
        // No motion cases:
        case 0b0000:
        case 0b1111:
        case 0b0101:
        case 0b1010:
            return 0;
        // Error if we're reading too slow.
        // Ignore for now since it is unlikely given how fast we're reading.
        default:
            return 0;
    }
        return 0; // should never happen.
}

// Update each encoder value based on new inputs and store in the write buffer.
void update_encoders()
{
    int32_t increment;
    // Read all encoder states simultaneously with one GPIO port read.
    // Note: encoder pins are contiguous along the GPIO port.
    //   i.e: Enc0A, Enc0B, Enc1A, Enc1B, ....
    //   EXCEPT the final encoder, which is on GPIO pins 26 and 27.
    curr_state = gpio_get_all() >> ENCODER_BASE_OFFSET;
    for (auto i=0;i<NUM_BMCS-1;++i)
    {
        // State machine logic.
        increment = get_encoder_increment(((prev_state >> 2*i) & 0x00000003),
                                          ((curr_state >> 2*i) & 0x00000003));
        // Write values into double buffer.
        write_buffer_ptr[i] = read_buffer_ptr[i] + increment;
    }
    // Handle final encoder case.
    uint32_t LAST_ENC_BASE = 14;
    increment = get_encoder_increment(((prev_state >> LAST_ENC_BASE) & 0x00000003),
                                      ((curr_state >> LAST_ENC_BASE) & 0x00000003));
    // Write values into double buffer.
    write_buffer_ptr[5] = read_buffer_ptr[5] + increment;
    prev_state = curr_state;
}


// Core1 reads encoders in a loop and manages their data in a double buffer.
// After some profiling, this code takes <100 cycles @ 133MHz, which means
// we can poll the encoders at ~1.33MHz.
void core1_main()
{
#ifdef PROFILE_ENCODER_LOOP
    uint32_t loop_start_systicks;
    uint32_t cpu_ticks;
    SYST_CSR |= (1 << 2) | (1 << 0); // Systick use cpu clock (133MHz). enable.
#endif
    // Clear encoder data.
    for (auto i=0;i<NUM_BMCS;++i)
    {
        read_buffer_ptr[i] = 0;
        write_buffer_ptr[i] = 0;
    }

    // Create starting state for encoders.
    prev_state = gpio_get_all() >> ENCODER_BASE_OFFSET;

    // Loop forever. Read Encoders and write new positions. Switch buffers.
    int32_t* tmp;
    while(true)
    {
#ifdef PROFILE_ENCODER_LOOP
        loop_start_systicks = SYST_CVR;
#endif
        update_encoders();
        // Switch buffers.
        tmp = write_buffer_ptr;
        write_buffer_ptr = read_buffer_ptr;
        read_buffer_ptr = tmp;
#ifdef PROFILE_ENCODER_LOOP
        cpu_ticks = loop_start_systicks - SYST_CVR; // CPU ticks count DOWN.
        printf("Encoder Loop CPU ticks: %d\r\n", cpu_ticks);
#endif
    }
}
