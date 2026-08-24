/*
 * Bounded synchronous example provider shared by core.echo and edu.probe.
 * It retains no request or buffer pointer and has no runtime dependencies.
 */

typedef unsigned char byte;
typedef unsigned int uint24;

typedef struct {
    byte size[2];
    byte abi_major;
    byte abi_minor;
    byte operation[2];
    byte flags[2];
    byte input[3];
    byte input_length[3];
    byte output[3];
    byte output_capacity[3];
    byte output_length[3];
    byte reserved;
} provider_request;

static uint24 read24(const byte value[3]) {
    return (uint24)value[0] | ((uint24)value[1] << 8) |
        ((uint24)value[2] << 16);
}

static void write24(byte value[3], uint24 number) {
    value[0] = number & 0xff;
    value[1] = (number >> 8) & 0xff;
    value[2] = (number >> 16) & 0xff;
}

__attribute__((section(".entry")))
uint24 emos_provider_entry(provider_request *request) {
    byte *input;
    byte *output;
    uint24 input_length;
    uint24 output_capacity;
    uint24 amount;
    uint24 index;

    if (!request || request->size[0] != sizeof(provider_request) ||
        request->size[1] != 0 || request->abi_major != 1 ||
        request->abi_minor != 0 || request->operation[0] != 2 ||
        request->operation[1] != 0 || request->reserved != 0)
        return 19; /* FR_INVALID_PARAMETER */
    input = (byte *)read24(request->input);
    output = (byte *)read24(request->output);
    input_length = read24(request->input_length);
    output_capacity = read24(request->output_capacity);
    if ((input_length && !input) || (output_capacity && !output))
        return 19;
    amount = input_length < output_capacity ? input_length : output_capacity;
    for (index = 0; index < amount; index++) output[index] = input[index];
    write24(request->output_length, amount);
    return 0;
}
