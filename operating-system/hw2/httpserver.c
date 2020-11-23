#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"

#define MAX_DATA_SIZE 10000
#define MAX_ADDR 1024
/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

typedef struct thread_argument{
    int id;
    void (*function)(int, int);
} arg_t;

int *lights;

void *garcon_forever(void* args){
  arg_t *arg = (arg_t *) args;
  int id = arg->id;
  int fd = 0;
  void (*make_me_proud)(int, int) = arg->function;
  while(1){
    fd = wq_pop(&work_queue);
    make_me_proud(fd, id);
    close(fd);
  }
}

void *do_proxy(void* args){
  int *arg = (int *)args;
  int id = arg[0];
  int input = arg[1];
  int output = arg[2];

  char data[MAX_DATA_SIZE];
  ssize_t len;

  while(lights[id] == 0 && (len = (ssize_t) read(input, data, sizeof(data) - 1)) > 0 ){
    http_send_data(output, data, (size_t)len);
  }
  (lights[id])++;

  return NULL;
}



void http_send_file(char* path, int http_fd, struct stat *status){
  int fd = open(path, O_RDONLY);
  char length[10];
  if (fd == -1){
    http_start_response(http_fd, 404);
    http_end_headers(http_fd);
    return;
  }
  http_start_response(http_fd, 200);
  http_send_header(http_fd, "Content-Type", http_get_mime_type(path));
  sprintf(length, "%lld", (long long int)status->st_size);
  http_send_header(http_fd, "Content-Length", length);
  http_end_headers(http_fd);

  char sent_file[MAX_DATA_SIZE];
  size_t file_size;
  while ((file_size = (size_t)read(fd, sent_file, MAX_DATA_SIZE)) > 0) {    //TODO: IS this bad?
    http_send_data(http_fd, sent_file, file_size);
  }
}

void http_send_dir(char* path, int fd){
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", "text/html");
  http_end_headers(fd);
  DIR *directory;
  struct dirent *directory_struct;
  char link[MAX_ADDR];
  http_send_string(fd, "<!DOCTYPE html>\n<html>\n<head>\n</head>\n<body>\n");
  if ((directory = opendir(path)) != NULL) {
    while ((directory_struct = readdir(directory)) != NULL) {
//      char *link = malloc((size_t)pathconf(".", _PC_PATH_MAX));
      strcpy(link, "<a href=\"");
      strcat(link, directory_struct->d_name);
      strcat(link, "\"><p>");
      strcat(link, directory_struct->d_name);
      strcat(link, "</p></a>\n");
      http_send_string(fd, link);
    }
    closedir(directory);
    http_send_string(fd, "</body>\n</html>");
  }

}
/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd, int id) {

  /*
   * TODO: Your solution for Task 1 goes here! Feel free to delete/modify *
   * any existing code.
   */

  struct http_request *request = http_request_parse(fd);

  if(request == NULL){
    http_start_response(fd, 404);
    http_end_headers(fd);
    return;
  }
  char full_path[MAX_ADDR];
  char index_path[MAX_ADDR];
  strcpy(full_path, server_files_directory);  //TODO: Path may be corrupted.
  strcat(full_path, request->path);
  struct stat status;
//  stat(full_path, &status);
  if (stat(full_path, &status) != 0) {
    http_start_response(fd, 404);
    http_end_headers(fd);
    return;
  }
  if(S_ISREG(status.st_mode) != 0){
    http_send_file(full_path, fd, &status);
  }
  else if(S_ISDIR(status.st_mode) != 0){
    strcpy(index_path, full_path);
    strcat(index_path, "/index.html");
    int file_d = open(index_path, O_RDONLY);
    if (file_d == -1){
      http_send_dir(full_path, fd);
    }
    else{
      stat(index_path, &status);        //TODO: VERY IMP.
      http_send_file(index_path, fd, &status);
    }
  } else{
    http_start_response(fd, 404);
    http_end_headers(fd);
  }


//  http_start_response(fd, 200);
//  http_send_header(fd, "Content-Type", "text/html");
//  http_end_headers(fd);
//  http_send_string(fd,
//      "<center>"
//      "<h1>Welcome to httpserver!</h1>"
//      "<hr>"
//      "<p>Nothing's here yet.</p>"
//      "</center>");
}


/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd, int id) {

  /*
  * The code below does a DNS lookup of server_proxy_hostname and 
  * opens a connection to it. Please do not modify.
  */

  int Tx_numbers[3], Rx_numbers[3];
  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  struct hostent *target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  int client_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (client_socket_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  if (target_dns_entry == NULL) {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    exit(ENXIO);
  }

  char *dns_address = target_dns_entry->h_addr_list[0];

  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status = connect(client_socket_fd, (struct sockaddr*) &target_address,
      sizeof(target_address));

  if (connection_status < 0) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    http_send_string(fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    return;

  }
  pthread_t Tx, Rx;

  Tx_numbers[0] = id;
  Tx_numbers[1] = client_socket_fd;
  Tx_numbers[2] = fd;

  Rx_numbers[0] = id;
  Rx_numbers[1] = fd;
  Rx_numbers[2] = client_socket_fd;

  lights[id] = 0;

  pthread_create(&Tx, NULL, do_proxy, (void *) Tx_numbers);
  pthread_create(&Rx, NULL, do_proxy, (void *) Rx_numbers);

  while (lights[id] == 0);

  pthread_cancel(Tx);
  pthread_cancel(Rx);

  close(client_socket_fd);
  /* 
  * TODO: Your solution for task 3 belongs here! 
  */
}


void init_thread_pool(int num_threads, void (*request_handler)(int, int)) {
  /*
   * TODO: Part of your solution for Task 2 goes here!
   */
  pthread_t threads[num_threads];
  int i;
  lights = calloc((size_t)num_threads, sizeof(int));

  for(i = 0 ; i < num_threads ; i++){
    arg_t *args = malloc(sizeof(arg_t));
    args->function = request_handler;
    args->id = i;
    pthread_create(&threads[i], NULL, garcon_forever, (void *)args);
  }
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int, int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }


  printf("Listening on port %d...\n", server_port);
  wq_init(&work_queue, num_threads);
  init_thread_pool(num_threads, request_handler);

  while (1) {
    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    // TODO: Change me?
    // TODO: OK.
    if(num_threads == 0) {
      request_handler(client_socket_number, 0);
      close(client_socket_number);
    } else{
      wq_push(&work_queue, client_socket_number);
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);
  
  signal(SIGPIPE,SIG_IGN);  

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int, int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
