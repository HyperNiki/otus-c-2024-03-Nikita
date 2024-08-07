#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>
#include <yaml.h>
#include <limits.h>

#define DEFAULT_CFG_FILE "cfg.yaml"
#define PERR_EXIT(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define FILE_NAME_TOKEN "file_name"
#define SOCK_NAME_TOKEN "sock_name"
#define SOCK_NAME_LEN 64

typedef struct {
    char file_name[PATH_MAX + 1];
    char sock_name[SOCK_NAME_LEN];
} fszd_cfg_s;

fszd_cfg_s cfg;
static volatile sig_atomic_t int_received = 0;
static volatile sig_atomic_t hup_received = 0;
char* cfg_file_name = DEFAULT_CFG_FILE;

void read_yaml(FILE* fd) {
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        syslog(LOG_ERR, "unable to initialize yaml parser");
        return;
    }
    yaml_parser_set_input_file(&parser, fd);
    yaml_token_t token;
    int token_type = 0;
    bool read_file_name = false;
    bool read_sock_name = false;
    do {
        yaml_parser_scan(&parser, &token);
        token_type = token.type;
        switch (token_type) {
            case YAML_SCALAR_TOKEN: {
                if (read_file_name) {
                    strncpy(cfg.file_name, (char*)token.data.scalar.value, PATH_MAX);
                    read_file_name = false;
                } else if (read_sock_name) {
                    strncpy(cfg.sock_name, (char*)token.data.scalar.value, SOCK_NAME_LEN);
                    read_sock_name = false;
                }
                read_file_name = (0 == strcmp((char*)token.data.scalar.value, FILE_NAME_TOKEN));
                read_sock_name = (0 == strcmp((char*)token.data.scalar.value, SOCK_NAME_TOKEN));
                break;
            }
            default:; // do nothing
        }
        yaml_token_delete(&token);
    } while (token_type != YAML_STREAM_END_TOKEN);

    yaml_parser_delete(&parser);
}

void read_cfg(const char* cfg_file) {
    FILE* fd = fopen(cfg_file, "r");

    if (!fd) {
        syslog(LOG_ERR, "unable to open file [%s]", cfg_file);
        return;
    }
    read_yaml(fd);
    fclose(fd);
    syslog(LOG_INFO, "watching for %s", cfg.file_name);
}

const char* cfg_get_file_name(void) { return cfg.file_name; }
const char* cfg_get_sock_name(void) { return cfg.sock_name; }

void print_help(void) {
    printf("NAME:\n"
           "    fszd\n"
           "        File SiZe Daemon\n"
           "OPTIONS:\n"
           "    -c, --config\n"
           "        Configuration file path\n"
           "    -N, --no-daemon\n"
           "        Run without demonization\n"
           "    -h, --help\n"
           "        Print help\n");
}

void sig_handler(int sig) {
    if (SIGINT == sig) {
        int_received = 1;
        return;
    }
    if (SIGHUP == sig) {
        hup_received = 1;
        return;
    }
}

long get_file_size(const char* file_name) {
    struct stat st;
    if (-1 == stat(file_name, &st)) {
        syslog(LOG_ERR, "unable to read file: %s", file_name);
        return -1;
    }
    return st.st_size;
}

void run(void) {
    int listener = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener < 0) {
        syslog(LOG_ERR, "listen socket error");
        return;
    }
    const char* sock_name = cfg_get_sock_name();
    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, sock_name);

    if (bind(listener, (struct sockaddr*)&server, sizeof(struct sockaddr_un)) < 0) {
        close(listener);
        unlink(sock_name);
        syslog(LOG_ERR, "listen socket bind error");
        return;
    }

    if (listen(listener, 8) < 0) {
        close(listener);
        unlink(sock_name);
        syslog(LOG_ERR, "listen error");
        return;
    }

    syslog(LOG_INFO, "start listening on %s", server.sun_path);
    int client = 0;
    char buf[1024];
    while (0 == int_received) {
        if (-1 == (client = accept(listener, 0, 0))) {
            if (EAGAIN == errno || EWOULDBLOCK == errno) {
                continue; // while
            }
            syslog(LOG_ERR, "client accept error");
            break; // while
        }
        const char* file_path = cfg_get_file_name();
        memset(buf, 0, sizeof(buf));
        if (get_file_size(file_path) != -1) 
            sprintf(buf, "%s size is [%ld] bytes\n", file_path, get_file_size(file_path));
        else
            sprintf(buf, "%s deleted\n", file_path);


        send(client, buf, strlen(buf), 0);
        close(client);

        if (1 == hup_received) {
            hup_received = 0;
            read_cfg(cfg_file_name);
        }
    }
    close(listener);
    unlink(sock_name);
}

// by W.Richard Stevens. Advanced Programming in the UNIX Environment
void daemonize(void) {
    umask(0); // clear file creation mask

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) { // get max num of file descriptors
        PERR_EXIT("getrlimit");
    }

    pid_t pid;
    if ((pid = fork()) < 0) {
        PERR_EXIT("1st fork failed");
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    setsid(); // become a session leader to lose controlling TTY

    if ((pid = fork()) < 0) {
        PERR_EXIT("2nd fork failed");
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (RLIM_INFINITY == rl.rlim_max) {
        rl.rlim_max = 1024;
    }

    for (rlim_t i = 0; i < rl.rlim_max; ++i) {
        close(i);
    }

    int daemon_in = open("/dev/null", O_RDWR);
    assert(0 == daemon_in);
    dup(0); // daemon out
    dup(0); // daemon err
}

int main(int argc, char** argv) {

    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"config", required_argument, 0, 'c'},
        {"no-daemon", no_argument, 0, 'N'},
        {0, 0, 0, 0},
    };
    int opt_char = 0;
    bool no_daemon_mode = false;

    while (-1 != (opt_char = getopt_long(argc, argv, ":c:hN", opts, NULL))) {
        switch (opt_char) {
            case 'c': {
                cfg_file_name = optarg;
                break;
            }
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'N': {
                no_daemon_mode = true;
                break;
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
            case ':': {
                fprintf(stderr, "required argument missing: '%c'\n\n", optopt);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!no_daemon_mode) {
        daemonize();
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_handler;
    if (-1 == sigaction(SIGHUP, &sa, NULL) || -1 == sigaction(SIGINT, &sa, NULL)) {
        PERR_EXIT("sigaction");
    }

    assert(argc > 0);
    openlog(argv[0], LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "started");
    read_cfg(cfg_file_name);
    run();
    syslog(LOG_INFO, "stopped");
    closelog();
    exit(EXIT_SUCCESS);
}
