#ifndef DEMO_SCREENS_H
#define DEMO_SCREENS_H

#include "lcd_driver.h"
#include "vt100_terminal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t rate_x10;
    uint32_t ms_x10;
} benchmark_result_t;

typedef uint64_t (*demo_time_us_fn)(void);

void demo_set_time_fn(demo_time_us_fn fn);

void demo_show_color_test_screen(lcd_driver_t *drv);
void demo_draw_demo_screen(lcd_driver_t *drv);
void demo_run_full_redraw_fps_test(lcd_driver_t *drv, benchmark_result_t *result);
void demo_show_full_redraw_fps_test(lcd_driver_t *drv);
void demo_show_terminal_benchmark_results(lcd_driver_t *drv, uint16_t origin_y);

#ifdef __cplusplus
}
#endif

#endif
