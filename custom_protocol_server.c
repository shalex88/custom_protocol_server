#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

int listener_d;

/**
 * @brief Print error message, errno string and terminate a program with exit code 1
 * 
 * @param msg Custom error description
 */
void error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/**
 * @brief Signal handler for Ctrl+C interrupt. Closes the socket and exits the programm
 * 
 * @param sig OS signal to be handled
 */
void handle_shutdown(int sig)
{
    if (listener_d)
    {
        close(listener_d);
    }
    fprintf(stderr, "\nServer stopped by user!\n");
    exit(0);
}

/**
 * @brief Catch OS signal interrupt and invoke proper signal handler
 * 
 * @param sig OS signal to be handled 
 * @param handler Function pointer to signal handler
 * @return int Insvoke a signal handler
 */
int catch_signal(int sig, void (*handler)(int))
{
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    return sigaction(sig, &action, NULL);
}

/**
 * @brief Open a socket. If opening fails call for an error handler
 * 
 * @return int Socket number
 */
int open_listener_socket()
{
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        error("Can't open socket");
    }

    return s;
}

/**
 * @brief Bind port for communication. If fails call for an error handler
 * 
 * @param socket Socket to be used 
 * @param port Port number to be used for communucation 
 */
void bind_to_port(int socket, int port)
{
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t)htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    int reuse = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
    {
        error("Can't set the reuse option on the socket");
    }
    int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
    if (c == -1)
    {
        error("Can't bind to socket");
    }
}

/**
 * @brief Send string to socket. If sending fails print message with errno description
 * 
 * @param socket Socket to be used
 * @param s String message to be sent to a socket
 * @return int Bytes number sent
 */
int say(int socket, char *s)
{
    int result = send(socket, s, strlen(s), 0);
    if (result == -1)
    {
        fprintf(stderr, "%s: %s\n", "Error talking to the client", strerror(errno));
    }

    return result;
}

/**
 * @brief Read ascii data from socket to the buffer till the maximum size or till receiving \n
 * 
 * @param socket Socket to receive data from
 * @param buf Buffer to save data to
 * @param len Maximum bytes to read
 * @return int Bytes received from socket
 */
int read_in(int socket, char *buf, int len)
{
    char *s = buf;
    int slen = len;
    int c = recv(socket, s, slen, 0);
    while ((c > 0) && (s[c - 1] != '\n'))
    {
        s += c;
        slen -= c;
        c = recv(socket, s, slen, 0);
    }
    if (c < 0)
    {
        return c;
    }
    else if (c == 0)
    {
        buf[0] = '\0';
    }
    else
    {
        s[c - 1] = '\0';
    }

    return len - slen;
}

int main(int argc, char *argv[])
{
    if (catch_signal(SIGINT, handle_shutdown) == -1)
    {
        error("Can’t set the interrupt handler");
    }

    listener_d = open_listener_socket();

    bind_to_port(listener_d, 30000); /* Create socket on port 30000 */

    if (listen(listener_d, 10) == -1)
    {
        error("Can’t listen");
    }

    struct sockaddr_storage client_addr;
    unsigned int address_size = sizeof(client_addr);

    puts("Waiting for connection...");

    char buf[255];

    while (1)
    {
        int connect_d = accept(listener_d, (struct sockaddr *)&client_addr, &address_size);
        if (connect_d == -1)
        {
            error("Can’t open secondary socket");
        }
        if (say(connect_d,"Internet Knock-Knock Protocol Server\r\nVersion 1.0\r\nKnock! Knock!\r\n> ") != -1)
        {
            read_in(connect_d, buf, sizeof(buf));
            if (strncasecmp("Who's there?", buf, 12)) /* Check if received message is valid */
            {
                say(connect_d, "You should say 'Who's there?'!");
            }
            else /* received valid message */
            {
                if (say(connect_d, "Oscar\r\n> ") != -1)
                {
                    read_in(connect_d, buf, sizeof(buf));
                    if (strncasecmp("Oscar who?", buf, 10)) /* Check if received message is valid */
                    {
                        say(connect_d, "You should say 'Oscar who?'!\r\n");
                    }
                    else /* received valid message */
                    {
                        say(connect_d, "Oscar silly question, you get a silly answer\r\n");
                    }
                }
            }
            
        }
        close(connect_d);
    }
    return 0;

}