#include "serial_port.h"

#include <stdlib.h>

#ifdef _WIN32

/* ------------------------------------------------------------------ */
/*  Windows implementation                                            */
/* ------------------------------------------------------------------ */

#include <windows.h>

struct serial_port {
    HANDLE handle;
};

serial_port_t *serial_port_open(const char *device, uint32_t baud)
{
    serial_port_t *port;
    HANDLE h;
    DCB dcb;
    COMMTIMEOUTS timeouts;

    h = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                    OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        return NULL;
    }

    dcb.BaudRate = (DWORD)baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        return NULL;
    }

    /* Non-blocking reads: return immediately with whatever is available. */
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;

    if (!SetCommTimeouts(h, &timeouts)) {
        CloseHandle(h);
        return NULL;
    }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    port = (serial_port_t *)malloc(sizeof(*port));
    if (port == NULL) {
        CloseHandle(h);
        return NULL;
    }

    port->handle = h;
    return port;
}

void serial_port_close(serial_port_t *port)
{
    if (port == NULL) {
        return;
    }

    CloseHandle(port->handle);
    free(port);
}

int serial_port_read(serial_port_t *port, void *buf, size_t len)
{
    DWORD bytes_read = 0;

    if (!ReadFile(port->handle, buf, (DWORD)len, &bytes_read, NULL)) {
        return -1;
    }

    return (int)bytes_read;
}

int serial_port_write(serial_port_t *port, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t remaining = len;

    while (remaining > 0) {
        DWORD written = 0;

        if (!WriteFile(port->handle, p, (DWORD)remaining, &written, NULL)) {
            return -1;
        }

        p += written;
        remaining -= written;
    }

    return 0;
}

#else

/* ------------------------------------------------------------------ */
/*  POSIX implementation                                              */
/* ------------------------------------------------------------------ */

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

struct serial_port {
    int fd;
};

static speed_t baud_to_speed(uint32_t baud)
{
    switch (baud) {
    case 1200:   return B1200;
    case 2400:   return B2400;
    case 4800:   return B4800;
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default:     return (speed_t)-1;
    }
}

serial_port_t *serial_port_open(const char *device, uint32_t baud)
{
    serial_port_t *port;
    struct termios tty;
    speed_t speed;
    int fd;

    speed = baud_to_speed(baud);
    if (speed == (speed_t)-1) {
        return NULL;
    }

    fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return NULL;
    }

    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return NULL;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~(tcflag_t)CSIZE) | CS8;
    tty.c_cflag &= ~(tcflag_t)(PARENB | PARODD);
    tty.c_cflag &= ~(tcflag_t)CSTOPB;
#ifdef CRTSCTS
    tty.c_cflag &= ~(tcflag_t)CRTSCTS;
#endif
    tty.c_cflag |= (tcflag_t)(CLOCAL | CREAD);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return NULL;
    }

    tcflush(fd, TCIOFLUSH);

    port = (serial_port_t *)malloc(sizeof(*port));
    if (port == NULL) {
        close(fd);
        return NULL;
    }

    port->fd = fd;
    return port;
}

void serial_port_close(serial_port_t *port)
{
    if (port == NULL) {
        return;
    }

    close(port->fd);
    free(port);
}

int serial_port_read(serial_port_t *port, void *buf, size_t len)
{
    ssize_t n = read(port->fd, buf, len);

    if (n >= 0) {
        return (int)n;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0;
    }

    return -1;
}

int serial_port_write(serial_port_t *port, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t n = write(port->fd, p, remaining);

        if (n > 0) {
            p += n;
            remaining -= (size_t)n;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}

#endif
