#include "libbb.h"

// Define the storage required for variables referenced as extern in the header
struct globals *ptr_to_globals = NULL;
uint32_t option_mask32 = 0;
const char *applet_name = "vi";

// Declare the external entry point from vi.c
int vi_main(int argc, char **argv);

int main(int argc, char **argv) {
    return vi_main(argc, argv);
}
