#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct serial_port serial_port_t;

serial_port_t *serial_port_open(const char *device, uint32_t baud);
void serial_port_close(serial_port_t *port);

/* Non-blocking read.  Returns bytes read, 0 if nothing available, -1 on error. */
int serial_port_read(serial_port_t *port, void *buf, size_t len);

/* Blocking (draining) write.  Returns 0 on success, -1 on error. */
int serial_port_write(serial_port_t *port, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
