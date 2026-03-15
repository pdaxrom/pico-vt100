#include "demo_screens.h"
#include "drivers/sdl2/sdl2_lcd.h"
#include "lcd_jpeg.h"
#include "lcd_text.h"
#include "serial_port.h"
#include "vt100_terminal.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifndef _WIN32
static int g_listen_fd = -1;
static int g_client_fd = -1;
static const char *g_socket_path = NULL;
#endif

static serial_port_t *g_serial_port = NULL;

static bool sdl2_delay_with_events(uint32_t ms)
{
    const uint32_t start = SDL_GetTicks();

    while (SDL_GetTicks() - start < ms) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                return false;
            }
        }

        SDL_Delay(16);
    }

    return true;
}

static uint64_t host_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

#ifndef _WIN32
static int create_listen_socket(const char *path)
{
    struct sockaddr_un addr;
    int fd;

    unlink(path);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0) {
        close(fd);
        unlink(path);
        return -1;
    }

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

static int accept_client(void)
{
    int fd;

    if (g_listen_fd < 0) {
        return -1;
    }

    fd = accept(g_listen_fd, NULL, NULL);
    if (fd < 0) {
        return -1;
    }

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

static void socket_send(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t remaining = len;

    if (g_client_fd < 0) {
        return;
    }

    while (remaining > 0) {
        ssize_t n = write(g_client_fd, p, remaining);

        if (n > 0) {
            p += n;
            remaining -= (size_t)n;
        } else if (n < 0 && errno == EAGAIN) {
            continue;
        } else {
            break;
        }
    }
}
#endif

static void remote_send(const void *data, size_t len)
{
#ifndef _WIN32
    if (g_client_fd >= 0) {
        socket_send(data, len);
        return;
    }
#endif

    if (g_serial_port != NULL) {
        serial_port_write(g_serial_port, data, len);
    }
}

static bool has_remote_connection(void)
{
#ifndef _WIN32
    if (g_client_fd >= 0) {
        return true;
    }
#endif

    return g_serial_port != NULL;
}

static void remote_terminal_output(const char *data, size_t len, void *user_data)
{
    (void)user_data;

    remote_send(data, len);
}

static bool remote_getch_hook(vt100_terminal_t *terminal, char ch, void *user_data)
{
    (void)terminal;
    (void)user_data;

    remote_send(&ch, 1);
    return true;
}

static void show_boot_logo(lcd_driver_t *drv, const char *path)
{
    FILE *f = NULL;
    uint8_t *buf = NULL;
    long size = 0;

    if (path == NULL) {
        return;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        return;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 512 * 1024) {
        fclose(f);
        return;
    }

    buf = (uint8_t *)malloc((size_t)size);
    if (buf == NULL) {
        fclose(f);
        return;
    }

    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return;
    }

    fclose(f);

    if (lcd_jpeg_draw(drv, buf, (size_t)size, 0u, 0u)) {
        sdl2_lcd_present(drv);
        sdl2_delay_with_events(3000);
    }

    free(buf);
}

static void sdl2_terminal_output(const char *data, size_t len, void *user_data)
{
    (void)user_data;

    for (size_t i = 0; i < len; ++i) {
        putchar(data[i]);
    }

    fflush(stdout);
}

static void send_escape_sequence(vt100_terminal_t *terminal, const char *seq)
{
    if (has_remote_connection()) {
        remote_send(seq, strlen(seq));
    } else {
        while (*seq != '\0') {
            vt100_terminal_putc(terminal, *seq++);
        }
    }
}

static void handle_sdl_key(vt100_terminal_t *terminal, const SDL_KeyboardEvent *key)
{
    const bool ctrl = (key->keysym.mod & KMOD_CTRL) != 0;
    const SDL_Keycode sym = key->keysym.sym;

    /* Escape sequences for special keys (bypass getch, go straight to parser). */
    switch (sym) {
    case SDLK_UP:    send_escape_sequence(terminal, "\x1b[A"); return;
    case SDLK_DOWN:  send_escape_sequence(terminal, "\x1b[B"); return;
    case SDLK_RIGHT: send_escape_sequence(terminal, "\x1b[C"); return;
    case SDLK_LEFT:  send_escape_sequence(terminal, "\x1b[D"); return;
    case SDLK_HOME:  send_escape_sequence(terminal, "\x1b[H"); return;
    case SDLK_END:   send_escape_sequence(terminal, "\x1b[F"); return;
    case SDLK_INSERT:   send_escape_sequence(terminal, "\x1b[2~"); return;
    case SDLK_DELETE:   send_escape_sequence(terminal, "\x1b[3~"); return;
    case SDLK_PAGEUP:   send_escape_sequence(terminal, "\x1b[5~"); return;
    case SDLK_PAGEDOWN: send_escape_sequence(terminal, "\x1b[6~"); return;
    case SDLK_F1:  send_escape_sequence(terminal, "\x1bOP"); return;
    case SDLK_F2:  send_escape_sequence(terminal, "\x1bOQ"); return;
    case SDLK_F3:  send_escape_sequence(terminal, "\x1bOR"); return;
    case SDLK_F4:  send_escape_sequence(terminal, "\x1bOS"); return;
    case SDLK_F5:  send_escape_sequence(terminal, "\x1b[15~"); return;
    case SDLK_F6:  send_escape_sequence(terminal, "\x1b[17~"); return;
    case SDLK_F7:  send_escape_sequence(terminal, "\x1b[18~"); return;
    case SDLK_F8:  send_escape_sequence(terminal, "\x1b[19~"); return;
    case SDLK_F9:  send_escape_sequence(terminal, "\x1b[20~"); return;
    case SDLK_F10: send_escape_sequence(terminal, "\x1b[21~"); return;
    case SDLK_F11: send_escape_sequence(terminal, "\x1b[23~"); return;
    case SDLK_F12: send_escape_sequence(terminal, "\x1b[24~"); return;
    default:
        break;
    }

    /* Control characters — fed through getch for local UI handling (Ctrl+E, etc.). */
    if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
        (void)vt100_terminal_getch(terminal, '\r');
        return;
    }

    if (sym == SDLK_BACKSPACE) {
        (void)vt100_terminal_getch(terminal, 0x7F);
        return;
    }

    if (sym == SDLK_TAB) {
        (void)vt100_terminal_getch(terminal, '\t');
        return;
    }

    if (sym == SDLK_ESCAPE) {
        (void)vt100_terminal_getch(terminal, 0x1B);
        return;
    }

    if (ctrl && sym >= SDLK_a && sym <= SDLK_z) {
        (void)vt100_terminal_getch(terminal, (int)(sym - SDLK_a + 1));
        return;
    }
}

int main(int argc, char *argv[])
{
    static vt100_terminal_t terminal;
    lcd_driver_t *drv = NULL;
    const char *socket_path = NULL;
    const char *serial_device = NULL;
    uint32_t serial_baud = 115200;
    uint16_t terminal_origin_y = 0;
    uint32_t last_tick_ms = 0;
    bool running = true;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--socket") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --socket requires a path argument\n\n");
                goto usage_error;
            }

            socket_path = argv[++i];
        } else if (strcmp(argv[i], "--serial-port") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --serial-port requires a device path\n\n");
                goto usage_error;
            }

            serial_device = argv[++i];
        } else if (strcmp(argv[i], "--serial-baud") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --serial-baud requires a baud rate\n\n");
                goto usage_error;
            }

            serial_baud = (uint32_t)strtoul(argv[++i], NULL, 10);
            if (serial_baud == 0) {
                fprintf(stderr, "Error: invalid baud rate\n\n");
                goto usage_error;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            fprintf(stdout, "Usage: %s [OPTIONS]\n\n", argv[0]);
            fprintf(stdout, "Options:\n");
            fprintf(stdout, "  --socket <path>          Unix socket for terminal I/O\n");
            fprintf(stdout, "  --serial-port <device>   Serial port for terminal I/O (8N1)\n");
            fprintf(stdout, "  --serial-baud <rate>     Baud rate (default: 115200)\n");
            fprintf(stdout, "  -h, --help               Show this help message\n");
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
            goto usage_error;
        }
    }

    if (socket_path != NULL && serial_device != NULL) {
        fprintf(stderr, "Error: --socket and --serial-port are mutually exclusive\n\n");
        goto usage_error;
    }

    if (0) {
usage_error:
        fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
        fprintf(stderr, "  --socket <path>          Unix socket for terminal I/O\n");
        fprintf(stderr, "  --serial-port <device>   Serial port for terminal I/O (8N1)\n");
        fprintf(stderr, "  --serial-baud <rate>     Baud rate (default: 115200)\n");
        fprintf(stderr, "  -h, --help               Show this help message\n");
        return 1;
    }

    demo_set_time_fn(host_time_us);

    drv = lcd_init();
    if (drv == NULL) {
        fprintf(stderr, "Failed to initialize LCD\n");
        return 1;
    }

#ifdef LOGO_JPG_PATH
    show_boot_logo(drv, LOGO_JPG_PATH);
#endif

    demo_show_color_test_screen(drv);
    sdl2_lcd_present(drv);
    sdl2_delay_with_events(2000);

    demo_draw_demo_screen(drv);
    sdl2_lcd_present(drv);
    sdl2_delay_with_events(2000);

    terminal_origin_y = (uint16_t)((lcd_height(drv) - VT100_TERMINAL_HEIGHT_PIXELS) / 2u);
    lcd_fill_screen(drv, LCD_COLOR_BLACK);
    vt100_terminal_init(&terminal, drv, 0u, terminal_origin_y);
    vt100_terminal_set_output(&terminal, sdl2_terminal_output, NULL);
    (void)vt100_terminal_getch(&terminal, -1);

#ifndef _WIN32
    if (socket_path != NULL) {
        g_socket_path = socket_path;
        g_listen_fd = create_listen_socket(socket_path);
        if (g_listen_fd < 0) {
            fprintf(stderr, "Failed to create socket: %s\n", socket_path);
            lcd_destroy(drv);
            return 1;
        }

        vt100_terminal_set_output(&terminal, remote_terminal_output, NULL);
        vt100_terminal_set_getch_hook(&terminal, remote_getch_hook, NULL);
    }
#endif

    if (serial_device != NULL) {
        g_serial_port = serial_port_open(serial_device, serial_baud);
        if (g_serial_port == NULL) {
            fprintf(stderr, "Failed to open serial port: %s at %u baud\n",
                    serial_device, (unsigned)serial_baud);
            lcd_destroy(drv);
            return 1;
        }

        vt100_terminal_set_output(&terminal, remote_terminal_output, NULL);
        vt100_terminal_set_getch_hook(&terminal, remote_getch_hook, NULL);
    }

    vt100_terminal_write(&terminal, "\x1b[2J\x1b[H");
    vt100_terminal_write(&terminal, "SDL2 VT100 TERMINAL 80X34 + STATUS\r\n");
    vt100_terminal_write(&terminal, "Last line is reserved for status. Ctrl+E enters local command mode.\r\n");
    vt100_terminal_write(&terminal, "Type text. Arrow keys, function keys, Page Up/Down supported.\r\n");

#ifndef _WIN32
    if (g_listen_fd >= 0) {
        char msg[128];

        snprintf(msg, sizeof(msg), "Listening on %s\r\n", socket_path);
        vt100_terminal_write(&terminal, msg);
    }
#endif

    if (g_serial_port != NULL) {
        char msg[128];

        snprintf(msg, sizeof(msg), "Serial: %s @ %u 8N1\r\n",
                 serial_device, (unsigned)serial_baud);
        vt100_terminal_write(&terminal, msg);
    }

    vt100_terminal_write(&terminal, "\r\n");
    vt100_terminal_write(&terminal, "\x1b[32mREADY\x1b[0m> ");
    sdl2_lcd_present(drv);

    last_tick_ms = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        const uint32_t now_ms = SDL_GetTicks();
        const uint32_t elapsed_ms = now_ms - last_tick_ms;

        last_tick_ms = now_ms;
        vt100_terminal_tick(&terminal, elapsed_ms);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                handle_sdl_key(&terminal, &event.key);
                break;
            case SDL_TEXTINPUT:
                for (const char *p = event.text.text; *p != '\0'; ++p) {
                    (void)vt100_terminal_getch(&terminal, (unsigned char)*p);
                }
                break;
            }
        }

#ifndef _WIN32
        if (g_listen_fd >= 0 && g_client_fd < 0) {
            int fd = accept_client();

            if (fd >= 0) {
                g_client_fd = fd;
                vt100_terminal_write(&terminal, "\r\n\x1b[33m[client connected]\x1b[0m\r\n");
            }
        }

        if (g_client_fd >= 0) {
            uint8_t buf[512];
            ssize_t n = read(g_client_fd, buf, sizeof(buf));

            if (n > 0) {
                vt100_terminal_write_n(&terminal, (const char *)buf, (size_t)n);
            } else if (n == 0) {
                vt100_terminal_write(&terminal, "\r\n\x1b[31m[client disconnected]\x1b[0m\r\n");
                close(g_client_fd);
                g_client_fd = -1;
            }
        }
#endif

        if (g_serial_port != NULL) {
            uint8_t buf[512];
            int n = serial_port_read(g_serial_port, buf, sizeof(buf));

            if (n > 0) {
                vt100_terminal_write_n(&terminal, (const char *)buf, (size_t)n);
            } else if (n < 0) {
                vt100_terminal_write(&terminal, "\r\n\x1b[31m[serial port error]\x1b[0m\r\n");
                serial_port_close(g_serial_port);
                g_serial_port = NULL;
            }
        }

        (void)vt100_terminal_getch(&terminal, -1);
        sdl2_lcd_present(drv);
        SDL_Delay(16);
    }

#ifndef _WIN32
    if (g_client_fd >= 0) {
        close(g_client_fd);
    }

    if (g_listen_fd >= 0) {
        close(g_listen_fd);
    }

    if (g_socket_path != NULL) {
        unlink(g_socket_path);
    }
#endif

    serial_port_close(g_serial_port);
    lcd_destroy(drv);
    return 0;
}
