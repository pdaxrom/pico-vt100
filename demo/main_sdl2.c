#include "demo_screens.h"
#include "drivers/sdl2/sdl2_lcd.h"
#include "lcd_jpeg.h"
#include "lcd_text.h"
#include "vt100_terminal.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    while (*seq != '\0') {
        vt100_terminal_putc(terminal, *seq++);
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
    uint16_t terminal_origin_y = 0;
    uint32_t last_tick_ms = 0;
    bool running = true;

    (void)argc;
    (void)argv;

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
    vt100_terminal_write(&terminal, "\x1b[2J\x1b[H");
    vt100_terminal_write(&terminal, "SDL2 VT100 TERMINAL 80X34 + STATUS\r\n");
    vt100_terminal_write(&terminal, "Last line is reserved for status. Ctrl+E enters local command mode.\r\n");
    vt100_terminal_write(&terminal, "Type text. Arrow keys, function keys, Page Up/Down supported.\r\n");
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

        (void)vt100_terminal_getch(&terminal, -1);
        sdl2_lcd_present(drv);
        SDL_Delay(16);
    }

    lcd_destroy(drv);
    return 0;
}
