#ifndef TERMI_EMULATOR_WRAPPER_H
#define TERMI_EMULATOR_WRAPPER_H

#include <stddef.h>

typedef void* EmulatorHandle;

int emulator_init_filesystem(const char *db_path, const char *data_path);
void emulator_deinit_filesystem(void);

EmulatorHandle emulator_create(void);
void emulator_destroy(EmulatorHandle handle);

int emulator_load_shell(EmulatorHandle handle, const char *shell_path);
int emulator_run_async(EmulatorHandle handle);
void emulator_stop(EmulatorHandle handle);

int emulator_send_input(EmulatorHandle handle, const char *data, size_t len);
int emulator_read_output(EmulatorHandle handle, char *buffer, size_t max_len);
int emulator_has_output(EmulatorHandle handle);

#endif
