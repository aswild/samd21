#include <ctype.h>

#include "Arduino.h"
#include "wiring_digital.h"

#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

extern void leds_init(void);
extern void leds_set_brightness(uint8_t b);
extern void leds_set_state(int state);

static constexpr unsigned long LOOP_DELAY_MS = 10;

static struct {
    int pin;
    int last = 0;
} inputs[] = {
    { .pin = 9 },
    { .pin = 8 },
};
static constexpr int N_INPUTS = sizeof(inputs) / sizeof(inputs[0]);

void setup(void)
{
    DBGINIT();
    DBGHIGH();

    for (int i = 0; i < N_INPUTS; i++)
        pinMode(inputs[i].pin, INPUT_PULLUP);

    leds_init();

    //while (!SerialUSB);
    //SerialUSB.print("Serial Events Test\r\n");
    DBGLOW();
}

static void handle_cmd(const char *cmd)
{
    int arg = -1;
    if (sscanf(cmd, "L %d", &arg) == 1)
    {
        leds_set_state(arg);
        SerialUSB.printf("MSG set LED state %d\r\n", arg);
    }
    else if (sscanf(cmd, "B %d", &arg) == 1)
    {
        const uint8_t b = static_cast<uint8_t>(arg);
        leds_set_brightness(b);
        SerialUSB.printf("MSG set brightness %u\r\n", b);
    }
    else
    {
        SerialUSB.printf("MSG Unknown Command '%s'\r\n", cmd);
    }
}

void loop(void)
{
    static constexpr size_t CMD_SIZE = 32;
    static char cmd[CMD_SIZE] = {0};
    static size_t cmd_pos = 0;

    // check for and handle commands
    while (SerialUSB.available())
    {
        char c = static_cast<char>(SerialUSB.read());
        if ((cmd_pos == 0) && isspace(c))
            continue; // ignore leading space in command
        else if (c == '\r')
            cmd[cmd_pos++] = '\0';
        else
            cmd[cmd_pos++] = toupper(c);

        if (c == '\r' || cmd_pos == CMD_SIZE)
        {
            // we have a complete command
            cmd[CMD_SIZE-1] = '\0'; // null-terminate in case we hit the limit
            handle_cmd(cmd);
            cmd_pos = 0;
        }
    }

    // handle input events
    for (int i = 0; i < N_INPUTS; i++)
    {
        const int current = !digitalRead(inputs[i].pin);
        if (current != inputs[i].last)
        {
            SerialUSB.printf("INPUT %d %d\r\n", i, current);
            inputs[i].last = current;
        }
    }

    delay(LOOP_DELAY_MS);
}
