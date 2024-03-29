#include <user_io_handler.h>

UserIOHandler::UserIOHandler()
:new_msg_{false}, buff_index_{0},
 msg_is_malformed_{false},
 parsed_msg_{ParsedUserMsg{}}
{
}

UserIOHandler::~UserIOHandler()
{
}


void UserIOHandler::init()
{
    stdio_usb_init();
    stdio_set_translate_crlf(&stdio_usb, false); // Don't replace outgoing chars.
    while (!stdio_usb_connected()){} // Block until pc connects to serial port.
}


void UserIOHandler::read_chars_nonblocking()
{
    // TODO: convert to do-while.
    uint new_char = getchar_timeout_us(0);
    while (new_char != PICO_ERROR_TIMEOUT)
    {
        raw_buffer_[buff_index_++] = char(new_char);
        new_char = getchar_timeout_us(0);
    }
    char& last_char = raw_buffer_[buff_index_-1];
    if (last_char == '\r' || last_char == '\n')
    {
        new_msg_ = true;
    }
    // Null terminate what we received for direct buffer printing.
    raw_buffer_[buff_index_] = '\0';
}


void UserIOHandler::parse_msg()
{
    // Tokenize the input into: [CMD] [motors] [motor values].
    // Interpret subtokens for motors as: [motor0] [motor1] ... [motorN]
    // and duty cycles for each motor as: [duty0] [duty1] ... [dutyN]
    // Stuff all the relevant data into parsed_msg_.

    // Safety guard to avoid parsing an unfinished string.
    if (!new_msg())
        return;

    char * ch_ptr;
    int token_count;
    char* tokens[MAX_TOKENS]; // Container to hold the start of all tokens.

    // Try-Catch everything so we can flag an error before exiting.
    try
    {
        token_count = extract_tokens(raw_buffer_, " \r\n", tokens, MAX_TOKENS);
        if (token_count <= 0)
            throw std::invalid_argument("Token count must be nonzero.");
        parsed_msg_.cmd = cmd_str_to_cmd(tokens[0]);

        // CMD Checks.
        Cmd& cmd = parsed_msg_.cmd;
        if (cmd == ERROR)
            throw std::invalid_argument("Command is not valid.");
        // Bail-early for cmds without additional args.
        if (cmd == IS_BUSY || cmd == HOME_ALL || cmd == HOME_ALL_IN_PLACE)
            return;
        // Check arg count for remaining cmds with args.
        if (cmd == HOME || cmd == HOME_IN_PLACE)
        {
            if (token_count != 2)
                throw std::invalid_argument("Incorrect token count for homing cmd.");
        }
        else if (cmd == SET_SPEED)
        {
            if (token_count != 3)
                throw std::invalid_argument("Incorrect token count for setting speed.");
        }
        else // all other cmds.
        {
            if (token_count != 4)
                throw std::invalid_argument("Incorrect token count.");
        }

        // Extract Motors.
        int motor_count;
        char* motor_strs[NUM_BMCS]; // Container for ptrs to motor strings.
        motor_count = extract_tokens(tokens[1], ",", motor_strs, NUM_BMCS);
        if (motor_count < 0)
            throw std::invalid_argument("");
        parsed_msg_.motor_count = motor_count;
        for (uint8_t bmc_index = 0; bmc_index < motor_count; ++bmc_index)
        {
            parsed_msg_.motor_indexes[bmc_index] =
                std::stoi(motor_strs[bmc_index]);
        }

        // Bail-early for cmds without additional args.
        if (cmd == HOME || cmd == HOME_IN_PLACE)
            return;

        // TODO: clean up this next section.
        if (cmd == SET_SPEED)
        {
            // Extract Motor Settings (Duty cycle, time in ms, etc.).
            int motor_arg_count;
            char* motor_vals_strs[NUM_BMCS]; // Container for ptrs to motor strings.
            motor_arg_count = extract_tokens(tokens[2], ",", motor_vals_strs,
                                             NUM_BMCS);
            if (motor_arg_count < 0 || motor_arg_count != parsed_msg_.motor_count)
                throw std::invalid_argument("");
            for (uint8_t bmc_index = 0; bmc_index < motor_arg_count; ++bmc_index)
            {
                parsed_msg_.motor_values[bmc_index] =
                    std::stoi(motor_vals_strs[bmc_index]);
            }
        }

        if (cmd == TIME_MOVE || cmd == DIST_MOVE)
        {
            int motor_dir_count;
            char* motor_dirs_strs[NUM_BMCS]; // Container for ptrs to dir strings.
            motor_dir_count = extract_tokens(tokens[2], ",", motor_dirs_strs,
                                             NUM_BMCS);
            if (motor_dir_count < 0 || motor_dir_count != parsed_msg_.motor_count)
                throw std::invalid_argument("");
            for (uint8_t bmc_index = 0; bmc_index < motor_dir_count; ++bmc_index)
            {
                parsed_msg_.directions[bmc_index] =
                    MotorController::dir_t(std::stoi(motor_dirs_strs[bmc_index]));
            }

            // Extract Motor Settings (Duty cycle, time in ms, etc.).
            int motor_arg_count;
            char* motor_vals_strs[NUM_BMCS]; // Container for ptrs to motor strings.
            motor_arg_count = extract_tokens(tokens[3], ",", motor_vals_strs,
                                             NUM_BMCS);
            if (motor_arg_count < 0 || motor_arg_count != parsed_msg_.motor_count)
                throw std::invalid_argument("");
            for (uint8_t bmc_index = 0; bmc_index < motor_arg_count; ++bmc_index)
            {
                parsed_msg_.motor_values[bmc_index] =
                    std::stoi(motor_vals_strs[bmc_index]);
            }
        }

        // Data checks.

        // TODO:
        // check that data is signed/sized in the way that is command-appropriate
        // check that motor indices are unique
    }
    catch (std::invalid_argument& e) // stoi throws this and so do we.
    {
        msg_is_malformed_ = true;
        #ifdef DEBUG
        printf("Error User Input is invalid.\r\n");
        printf(e.what());
        #endif
    }
}


int UserIOHandler::extract_tokens(char input[], const char delimiters[],
                                   char* tokens[], size_t max_tokens)
{
    char* ch_ptr;
    int token_count = 0;
    // First strtok call takes the pointer to the start of the string.
    // Subsequent calls take NULL as input to keep tokenizing the same string.
    // Note: strtok destructively modifies the string!
    ch_ptr = strtok(input, delimiters);
    while (ch_ptr != NULL)
    {
        tokens[token_count] = ch_ptr;
        ++token_count;
        ch_ptr = strtok(NULL, delimiters);
        // Check if the user input too many tokens.
        if (token_count > max_tokens)
        {
            return -1;
        }
    }
    return token_count;
}


// TODO: consider a std::map with an embedded-friendly implementation.
Cmd UserIOHandler::cmd_str_to_cmd(char* cmd_str)
{
        if (std::strcmp(cmd_str, "IS_BUSY") == 0)
            return IS_BUSY;
        if (std::strcmp(cmd_str, "TIME_MOVE") == 0)
            return TIME_MOVE;
        if (std::strcmp(cmd_str, "DIST_MOVE") ==  0)
            return DIST_MOVE;
        if (std::strcmp(cmd_str, "HOME") == 0)
            return HOME;
        if (std::strcmp(cmd_str, "HOME_IN_PLACE") == 0)
            return HOME_IN_PLACE;
        if (std::strcmp(cmd_str, "HOME_ALL") == 0)
            return HOME_ALL;
        if (std::strcmp(cmd_str, "SET_SPEED") == 0)
            return SET_SPEED;
        if (std::strcmp(cmd_str, "HOME_ALL_IN_PLACE") == 0)
            return HOME_ALL_IN_PLACE;
        return ERROR;
}
