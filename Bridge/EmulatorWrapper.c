#include "EmulatorWrapper.h"
#include "../Emulator/emulator.h"
#include "../Filesystem/fakefs/fake.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define IO_BUFFER_SIZE 8192

struct fakefs global_fakefs;
static int global_fakefs_initialized = 0;

void *emulator_memory_base = NULL;
size_t emulator_memory_size = 0;

struct circular_buffer {
    char data[IO_BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
    size_t count;
    pthread_mutex_t mutex;
};

struct emulator_context {
    arm64_emulator_t *emu;
    pthread_t thread;
    char *elf_buffer;
    size_t elf_size;
    int running;
    int should_stop;
    struct circular_buffer input_buffer;
    struct circular_buffer output_buffer;
};

static void circular_buffer_init(struct circular_buffer *buf) {
    buf->read_pos = 0;
    buf->write_pos = 0;
    buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
}

static void circular_buffer_destroy(struct circular_buffer *buf) {
    pthread_mutex_destroy(&buf->mutex);
}

static int circular_buffer_write(struct circular_buffer *buf, const char *data, size_t len) {
    pthread_mutex_lock(&buf->mutex);
    
    size_t available = IO_BUFFER_SIZE - buf->count;
    if (len > available) {
        len = available;
    }
    
    size_t written = 0;
    while (written < len) {
        buf->data[buf->write_pos] = data[written];
        buf->write_pos = (buf->write_pos + 1) % IO_BUFFER_SIZE;
        written++;
        buf->count++;
    }
    
    pthread_mutex_unlock(&buf->mutex);
    return (int)written;
}

static int circular_buffer_read(struct circular_buffer *buf, char *data, size_t max_len) {
    pthread_mutex_lock(&buf->mutex);
    
    size_t to_read = buf->count < max_len ? buf->count : max_len;
    size_t read_count = 0;
    
    while (read_count < to_read) {
        data[read_count] = buf->data[buf->read_pos];
        buf->read_pos = (buf->read_pos + 1) % IO_BUFFER_SIZE;
        read_count++;
        buf->count--;
    }
    
    pthread_mutex_unlock(&buf->mutex);
    return (int)read_count;
}

static int circular_buffer_available(struct circular_buffer *buf) {
    pthread_mutex_lock(&buf->mutex);
    int available = (int)buf->count;
    pthread_mutex_unlock(&buf->mutex);
    return available;
}

int emulator_init_filesystem(const char *db_path, const char *data_path) {
    printf("[EmulatorWrapper] Initializing filesystem: db=%s, data=%s\n", db_path, data_path);
    
    if (global_fakefs_initialized) {
        printf("[EmulatorWrapper] Filesystem already initialized\n");
        return 0;
    }
    
    int result = fakefs_init(&global_fakefs, db_path, data_path);
    if (result < 0) {
        printf("[EmulatorWrapper] Failed to initialize filesystem: %d\n", result);
        return -1;
    }
    
    global_fakefs_initialized = 1;
    printf("[EmulatorWrapper] Filesystem initialized successfully\n");
    return 0;
}

void emulator_deinit_filesystem(void) {
    printf("[EmulatorWrapper] Deinitializing filesystem\n");
    
    if (!global_fakefs_initialized) {
        return;
    }
    
    fakefs_deinit(&global_fakefs);
    global_fakefs_initialized = 0;
    printf("[EmulatorWrapper] Filesystem deinitialized\n");
}

EmulatorHandle emulator_create(void) {
    printf("[EmulatorWrapper] Creating emulator context\n");
    
    struct emulator_context *ctx = malloc(sizeof(struct emulator_context));
    if (!ctx) {
        printf("[EmulatorWrapper] Failed to allocate emulator context\n");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(struct emulator_context));
    
    ctx->emu = arm64_emulator_create();
    if (!ctx->emu) {
        printf("[EmulatorWrapper] Failed to create ARM64 emulator\n");
        free(ctx);
        return NULL;
    }
    
    circular_buffer_init(&ctx->input_buffer);
    circular_buffer_init(&ctx->output_buffer);
    
    ctx->running = 0;
    ctx->should_stop = 0;
    
    printf("[EmulatorWrapper] Emulator context created successfully\n");
    return (EmulatorHandle)ctx;
}

void emulator_destroy(EmulatorHandle handle) {
    printf("[EmulatorWrapper] Destroying emulator context\n");
    
    if (!handle) {
        return;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    
    if (ctx->running) {
        printf("[EmulatorWrapper] Stopping running emulator before destroy\n");
        emulator_stop(handle);
    }
    
    if (ctx->emu) {
        arm64_emulator_destroy(ctx->emu);
    }
    
    if (ctx->elf_buffer) {
        free(ctx->elf_buffer);
    }
    
    circular_buffer_destroy(&ctx->input_buffer);
    circular_buffer_destroy(&ctx->output_buffer);
    
    free(ctx);
    printf("[EmulatorWrapper] Emulator context destroyed\n");
}

int emulator_load_shell(EmulatorHandle handle, const char *shell_path) {
    printf("[EmulatorWrapper] Loading shell from: %s\n", shell_path);
    
    if (!handle || !shell_path) {
        printf("[EmulatorWrapper] Invalid parameters\n");
        return -1;
    }
    
    if (!global_fakefs_initialized) {
        printf("[EmulatorWrapper] Filesystem not initialized\n");
        return -1;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    
    struct stat statbuf;
    if (fakefs_stat(&global_fakefs, shell_path, &statbuf) < 0) {
        printf("[EmulatorWrapper] Failed to stat shell file: %s\n", shell_path);
        return -1;
    }
    
    size_t file_size = (size_t)statbuf.st_size;
    printf("[EmulatorWrapper] Shell file size: %zu bytes\n", file_size);
    
    int fd = fakefs_open(&global_fakefs, shell_path, O_RDONLY, 0);
    if (fd < 0) {
        printf("[EmulatorWrapper] Failed to open shell: %s (error: %d)\n", shell_path, fd);
        return -1;
    }
    
    char *buffer = malloc(file_size);
    if (!buffer) {
        printf("[EmulatorWrapper] Failed to allocate buffer for shell\n");
        fakefs_close(&global_fakefs, fd);
        return -1;
    }
    
    ssize_t bytes_read = fakefs_read(&global_fakefs, fd, buffer, file_size);
    fakefs_close(&global_fakefs, fd);
    
    if (bytes_read < 0 || (size_t)bytes_read != file_size) {
        printf("[EmulatorWrapper] Failed to read shell: read %zd of %zu bytes\n", bytes_read, file_size);
        free(buffer);
        return -1;
    }
    
    printf("[EmulatorWrapper] Loaded %zu bytes from shell, calling emulator load\n", file_size);
    
    int result = arm64_emulator_load_elf_memory_with_fs(ctx->emu, buffer, file_size, &global_fakefs);
    if (result < 0) {
        printf("[EmulatorWrapper] Failed to load ELF into emulator: %d\n", result);
        free(buffer);
        return -1;
    }
    
    ctx->elf_buffer = buffer;
    ctx->elf_size = file_size;
    
    printf("[EmulatorWrapper] Shell loaded successfully\n");
    return 0;
}

static void *emulator_thread_func(void *arg) {
    struct emulator_context *ctx = (struct emulator_context *)arg;
    
    printf("[EmulatorWrapper] Emulator thread starting\n");
    
    ctx->running = 1;
    
    arm64_emulator_run(ctx->emu);
    
    ctx->running = 0;
    
    printf("[EmulatorWrapper] Emulator thread finished\n");
    return NULL;
}

int emulator_run_async(EmulatorHandle handle) {
    printf("[EmulatorWrapper] Starting emulator asynchronously\n");
    
    if (!handle) {
        printf("[EmulatorWrapper] Invalid handle\n");
        return -1;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    
    if (ctx->running) {
        printf("[EmulatorWrapper] Emulator already running\n");
        return -1;
    }
    
    if (!ctx->elf_buffer) {
        printf("[EmulatorWrapper] No shell loaded\n");
        return -1;
    }
    
    int result = pthread_create(&ctx->thread, NULL, emulator_thread_func, ctx);
    if (result != 0) {
        printf("[EmulatorWrapper] Failed to create thread: %d\n", result);
        return -1;
    }
    
    printf("[EmulatorWrapper] Emulator thread started\n");
    return 0;
}

void emulator_stop(EmulatorHandle handle) {
    printf("[EmulatorWrapper] Stopping emulator\n");
    
    if (!handle) {
        return;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    
    if (!ctx->running) {
        printf("[EmulatorWrapper] Emulator not running\n");
        return;
    }
    
    ctx->should_stop = 1;
    
    pthread_join(ctx->thread, NULL);
    
    printf("[EmulatorWrapper] Emulator stopped\n");
}

int emulator_send_input(EmulatorHandle handle, const char *data, size_t len) {
    if (!handle || !data || len == 0) {
        return -1;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    return circular_buffer_write(&ctx->input_buffer, data, len);
}

int emulator_read_output(EmulatorHandle handle, char *buffer, size_t max_len) {
    if (!handle || !buffer || max_len == 0) {
        return -1;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    return circular_buffer_read(&ctx->output_buffer, buffer, max_len);
}

int emulator_has_output(EmulatorHandle handle) {
    if (!handle) {
        return 0;
    }
    
    struct emulator_context *ctx = (struct emulator_context *)handle;
    return circular_buffer_available(&ctx->output_buffer);
}
