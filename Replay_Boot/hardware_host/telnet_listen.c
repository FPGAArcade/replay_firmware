// gcc -DTEST -Wall -Wpedantic telnet_listen.c -o listen && ./listen

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int TELNET_socket = -1;
int TELNET_fd = -1;

int TELNET_init(uint16_t port)
{
    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if ((TELNET_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("failed to create listen socket (%i)\n", errno);
        return 1;
    }

    if ((setsockopt(TELNET_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int))) < 0) {
        printf("failed to set socket options(%i)\n", errno);
        return 1;
    }

    if ((bind(TELNET_socket, (struct sockaddr*)&addr_in, sizeof(addr_in))) < 0) {
        printf("failed to bind socket (%i)\n", errno);
        return 1;
    }

    int num_clients = 1;

    // dropping begins
    if (listen(TELNET_socket, num_clients) < 0) {
        printf("failed to open socket for listening (%i)\n", errno);
        return 1;
    }

    int saved_flags = fcntl(TELNET_socket, F_GETFL);
    fcntl(TELNET_socket, F_SETFL, saved_flags | O_NONBLOCK);

    return 0;
}



static ssize_t strip_telnet_commands(uint8_t* buffer, ssize_t len)
{
    // HACK - This assumes the incoming buffer holds the full telnet command message

    // See RFC 854
    const uint8_t SE   = 240;  // subnegotiation end
    const uint8_t GA   = 249;  // go ahead
    const uint8_t SB   = 250;  // subnegotiation begin
    const uint8_t WILL = 251;  // will (option)
    const uint8_t DONT = 254;  // don't (option)
    const uint8_t IAC  = 255;  // interpret as command

    for (ssize_t pos = 0; pos < len; ++pos) {

        // check for IAC
        if (buffer[pos] != IAC) {
            continue;
        }

        // sanity check; end of the buffer?
        if (++pos == len) {
            break;
        }

        uint8_t c = buffer[pos++];

        // minimum strip the IAC itself
        int bytes_to_strip = 1;

        if (SE <= c && c <= GA) {
            bytes_to_strip = 2;     // 2 bytes command

        } else if (WILL <= c && c <= DONT) {
            bytes_to_strip = 3;     // 3 bytes command

        } else if (c == SB) {

            // subnegotiation start (multi-byte)
            for (ssize_t next = pos; next < len; ++next) {

                // check for IAC
                if (buffer[next] != IAC) {
                    continue;
                }

                // sanity check; end of the buffer?
                if (++next == len) {
                    break;
                }

                // check for subnegotiation end
                if (buffer[next] == SE) {
                    bytes_to_strip = next - pos + 1;
                    break;
                }
            }
        }

        memmove(&buffer[pos - 2], &buffer[pos + bytes_to_strip - 2], len - bytes_to_strip);
        len -= bytes_to_strip;
        pos -= 3;
    }

    return len;
}


static uint8_t TELNET_buffer[TCP_MSS];
static ssize_t TELNET_data_available = 0;

bool TELNET_connected()
{
    return TELNET_fd >= 0;
}

size_t TELNET_recv(void* buffer, size_t size)
{
    if (!TELNET_connected() && (TELNET_fd = accept(TELNET_socket, NULL, NULL)) >= 0) {

        // See RFC 854 / 857 / 1184
        const uint8_t IAC_DO_LINEMODE[] = { 255, 253, 34 };
        const uint8_t IAC_WILL_ECHO[]   = { 255, 251, 1 };
        write(TELNET_fd, IAC_DO_LINEMODE, sizeof(IAC_DO_LINEMODE));
        write(TELNET_fd, IAC_WILL_ECHO, sizeof(IAC_WILL_ECHO));

        printf("client connected; char mode set\n");
    }

    if (TELNET_connected() && !TELNET_data_available) {
        ssize_t bytes_read = read(TELNET_fd, TELNET_buffer, sizeof(TELNET_buffer));

        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("error reading from socket (%i)\n", errno);
            }

        } else if (bytes_read == 0) {
            printf("client disconnected\n");
            close(TELNET_fd);
            TELNET_fd = -1;

        } else {
            TELNET_data_available = strip_telnet_commands(TELNET_buffer, bytes_read);
            printf("received data (%li -> %li)!\n", bytes_read, TELNET_data_available);
        }
    }

    if (TELNET_data_available) {
        if (TELNET_data_available < size) {
            size = TELNET_data_available;
        }

        memcpy(buffer, TELNET_buffer, size);
        TELNET_data_available -= size;
        memmove(TELNET_buffer, &TELNET_buffer[size], TELNET_data_available);
        return size;
    }

    return 0;
}

void TELNET_send(void* buffer, size_t size)
{
    if (TELNET_fd == -1) {
        return;
    }

    write(TELNET_fd, buffer, size);
}


void TELNET_term()
{
    if (TELNET_fd != -1) {
        printf("closing socket %i\n", TELNET_fd);
        close(TELNET_fd);
        TELNET_fd = -1;
    }

    if (TELNET_socket != -1) {
        printf("closing socket %i\n", TELNET_socket);
        close(TELNET_socket);
        shutdown(TELNET_socket, 2);
        TELNET_socket = -1;
    }
}

#if TEST

#include <ctype.h>
void DumpBuffer(const uint8_t* pBuffer, uint32_t size)
{
    printf("DumpBuffer: size = %d (0x%x) bytes\n", size, size);
    uint32_t i, j, len;
    char format[150];
    char alphas[27];
    strcpy(format, "\t0x%08X: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X ");

    for (i = 0; i < size; i += 16) {
        len = size - i;

        // last line is less than 16 bytes? rewrite the format string
        if (len < 16) {
            strcpy(format, "\t0x%08X: ");

            for (j = 0; j < 16; ++j) {
                if (j < len) {
                    strcat(format, "%02X");

                } else {
                    strcat(format, "__");
                }

                if ((j & 0x3) == 0x3) {
                    strcat(format, " ");
                }
            }

        } else {
            len = 16;
        }

        // create the ascii representation
        for (j = 0; j < len; ++j) {
            alphas[j] = (isalnum(pBuffer[i + j]) ? pBuffer[i + j] : '.');
        }

        for (; j < 16; ++j) {
            alphas[j] = '_';
        }

        alphas[j] = 0;

        j = strlen(format);
        sprintf(format + j, "'%s'", alphas);

        printf(format, i,
               pBuffer[i + 0], pBuffer[i + 1], pBuffer[i + 2], pBuffer[i + 3], pBuffer[i + 4], pBuffer[i + 5], pBuffer[i + 6], pBuffer[i + 7],
               pBuffer[i + 8], pBuffer[i + 9], pBuffer[i + 10], pBuffer[i + 11], pBuffer[i + 12], pBuffer[i + 13], pBuffer[i + 14], pBuffer[i + 15]);
        printf("\n");

        format[j] = '\0';
    }
}

static void print_busy()
{
    static uint8_t x = 0;
    const char busy[] = "-\\|/";
    printf("%c\r", busy[x++ & 3]);
    fflush(stdout);
    usleep(100000);
}

int main(int argc, char* argv[])
{
    const uint16_t port = 1234;

    if (TELNET_init(port)) {
        return 1;
    }

    uint8_t buffer[16];

    while (true) {

        size_t read = TELNET_recv(buffer, sizeof(buffer));

        if (!read) {
            print_busy();
            continue;
        }

        DumpBuffer(buffer, read);
        TELNET_send(buffer, read);
    }

    TELNET_term();
    return 0;
}

#endif
