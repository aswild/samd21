#include <ctype.h>

#include "Arduino.h"
#include "wiring_digital.h"

#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

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

    //while (!SerialUSB);
    //SerialUSB.print("Serial Events Test\r\n");
    DBGLOW();
}

static void handle_cmd(const char *cmd)
{
    SerialUSB.printf("Got command: '%s'\r\n", cmd);
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
            cmd[cmd_pos++] = c;

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
