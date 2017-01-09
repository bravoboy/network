#ifndef network_h_
#define network_h_

int network_set_reuse(int fd);
int network_set_block(int fd);
int network_set_noblock(int fd);
int network_set_tcpnodelay(int fd);
/*
 * @param reuse = 1 mean set tcp reuse
 */
int network_bind_listen(int port, int reuse);

/*
 * @param timeout_ms = -1 mean block connect
 */

int network_connet(const char *address, int port, int timeout_ms);

static int network_get_socket_error(int fd);
static int wait_ready(int fd, int msec);
#endif
