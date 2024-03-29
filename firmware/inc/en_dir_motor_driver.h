#ifndef EN_DIR_MOTOR_DRIVER_H
#define EN_DIR_MOTOR_DRIVER_H
#include <stdint.h>
#include <pico/stdlib.h>
#include <hardware/pwm.h>


/**
 * \brief class for motor controller with (Enable Pin, Direction Pin) interface.
 */
class EnDirMotorDriver
{
public:
    /**
     * \brief constructor. Connect to PWM hardware, set dir_pin to logic 0;
     *        set torque_pwm_pin to 0.
     * \param torque_pwm_pin the pwm output pin to the controller enable pin.
     * \param dir_pin the output pin to the controller dir pin.
     * \note torque_pwm_pin and dir_pin must be different pins.
     */
    EnDirMotorDriver(uint8_t torque_pwm_pin, uint8_t dir_pin);


    /**
     * \brief destructor. Leave the outputs off and configured as input.
     */
    ~EnDirMotorDriver();


    /**
     *\brief sets the duty cycle to a value between 0 (off) and 100 (fully on).
     */
    void set_duty_cycle(uint8_t duty_cycle_percentage);


    /**
     *\brief sets the pwm frequency in Hz. Between 5[KHz] and 500[KHz]
     */
    void set_pwm_frequency(uint32_t freq_hz);


    /**
     * \brief enable the pwm output
     * \note inline
     */
    void enable_output(void)
    {pwm_set_chan_level(slice_num_, gpio_channel_, duty_cycle_);}


    /**
     * \brief disable the pwm output
     * \note inline.
     */
    void disable_output(void)
    {pwm_set_chan_level(slice_num_, gpio_channel_, 0);}

    /**
     * \brief disable the pwm output
     * \note inline.
     */
    void set_dir(bool dir)
    {gpio_put(dir_pin_, dir);}

private:
    uint torque_pwm_pin_;
    uint slice_num_;
    uint gpio_channel_;
    uint dir_pin_;
    uint duty_cycle_; /// The current duty cycle setting.

    // Constants
    static const uint SYSTEM_CLOCK = 133000000;

    static const uint PWM_STEP_INCREMENTS = 100;
    static const uint DEFAULT_PWM_FREQ_HZ = 20000; // Just beyond human hearing.

    // PWM Frequency range bounds.
    static const uint DIVIDER_MIN_FREQ_HZ = 5000;
    static const uint DRIVER_MAX_FREQ_HZ = 500000;

};

#endif // EN_DIR_MOTOR_DRIVER_H
