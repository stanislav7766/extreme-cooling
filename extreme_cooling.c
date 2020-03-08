#include <stdint.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EC_SC 0x66
#define EC_DATA 0x62
#define IBF 1
#define OBF 0
#define EC_SC_READ_CMD 0x80
#define EC_SC_WRITE_CMD 0x81
#define EC_SC_SCI_CMD 0x84
#define EC_REG 0xBD
#define FANS_MAX 0x40
#define FANS_DEFAULT 0x00

static void init()
{
    if (ioperm(EC_DATA, 1, 1) != 0)
    {
        perror("ioperm(EC_DATA, 1, 1)");
        exit(1);
    }

    if (ioperm(EC_SC, 1, 1) != 0)
    {
        perror("ioperm(EC_SC, 1, 1)");
        exit(1);
    }
}

static void wait_ec(const uint32_t port, const uint32_t flag, const char value)
{
    uint8_t data;
    int i;

    i = 0;
    data = inb(port);

    while ( (((data >> flag) & 0x1) != value) && (i++ < 100) )
    {
        usleep(1000);
        data = inb(port);
    }

    if (i >= 100)
    {
        fprintf(stderr, "wait_ec error on port 0x%x, data=0x%x, flag=0x%x, value=0x%x\n", port, data, flag, value);
        exit(1);
    }
}

static uint8_t read_ec(const uint32_t port)
{
    uint8_t value;

    wait_ec(EC_SC, IBF, 0);
    outb(EC_SC_READ_CMD, EC_SC);
    wait_ec(EC_SC, IBF, 0);
    outb(port, EC_DATA);
    wait_ec(EC_SC, OBF, 1);
    value = inb(EC_DATA);

    return value;
}

static void write_ec(const uint32_t port, const uint8_t value)
{
    wait_ec(EC_SC, IBF, 0);
    outb(EC_SC_WRITE_CMD, EC_SC);
    wait_ec(EC_SC, IBF, 0);
    outb(port, EC_DATA);
    wait_ec(EC_SC, IBF, 0);
    outb(value, EC_DATA);
    wait_ec(EC_SC, IBF, 0);
}

static void dump_all_regs(void)
{
    uint8_t val;
    int i;

    printf("EC reg dump:");

    for (i = 0x00; i <= 0xff; i++)
    {
        if ((i % 16) == 0)
        {
            printf("\n 0x%02x: ", i);
        }

        val = read_ec(i);
        printf("%02x ", val);
    }

    printf("\n");
}

static void set_value(const uint8_t value)
{
    uint8_t rval;

    rval = read_ec(EC_REG);
    printf("old value %02x\n", rval);
    write_ec(EC_REG, value);
    rval = read_ec(EC_REG);
    printf("new value %02x\n", rval);
}

int main(int argc, char *argv[])
{
    init();

    if (argc < 2)
    {
        dump_all_regs();
        printf("\nOptions:\n");
        printf("\tstart - start extreme cooling\n");
        printf("\tstop - stop extreme cooling\n");
    }
    else
    {
        if (strcmp(argv[1], "start") == 0)
        {
            printf("extreme cooling started\n");
            set_value(FANS_MAX);
        }
        else if (strcmp(argv[1], "stop") == 0)
        {
            printf("extreme cooling stopped\n");
            set_value(FANS_DEFAULT);
        }
        else
        {
            printf("unknown option\n");
        }
    }

    return 0;
}