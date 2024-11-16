#include "shell.h"
#include "commands.h"
#include "parse.h"
#include <bits/posix1_lim.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

// Buffer Sizes
#define INPUT_BUFFER_SIZE 1024 // Buffer size for input in read_command()
#define PROMPT_BUFFER_SIZE 256 // Buffer size for the prompt in read_command()

// Job ID Initialization
#define INITIAL_JOB_ID 1 // Initial ID for job tracking in add_job()

// ANSI Colors
#define COLOR_RED                                                              \
  "\033[1;31m" // Color code for red, used in prompt and startup animation
#define COLOR_WHITE "\033[1;37m" // Color code for white, used in prompt
#define COLOR_GREEN                                                            \
  "\033[1;32m" // Color code for green, used in startup animation
#define COLOR_YELLOW                                                           \
  "\033[1;33m" // Color code for yellow, used in startup animation
#define COLOR_BLUE                                                             \
  "\033[1;34m" // Color code for blue, used in startup animation
#define COLOR_MAGENTA                                                          \
  "\033[1;35m" // Color code for magenta, used in startup animation
#define COLOR_CYAN                                                             \
  "\033[1;36m"                // Color code for cyan, used in startup animation
#define COLOR_RESET "\033[0m" // Reset color to default

// Delays for animate_startup (in microseconds)
#define BOOT_INIT_DELAY 1000000  // Delay for boot initialization
#define SYSTEM_LOAD_DELAY 700000 // Delay for system loading message
#define NETWORK_RECONNECT_DELAY                                                \
  1500000                            // Delay for network reconnecting message
#define RESOURCE_GATHER_DELAY 700000 // Delay for resource gathering message
#define READY_DELAY 700000           // Delay before ready message
#define FINAL_WELCOME_DELAY 500000   // Delay before final welcome message

/* Definicion de la cabeza de la lista de trabajos*/
static job_t *job_list = NULL;
static int next_job_id = INITIAL_JOB_ID;
pid_t foreground_pid = 0;
struct termios orig_termios;
volatile sig_atomic_t sigchld_flag = 0;

void sigchld_handler(int signo) { sigchld_flag = 1; }

void sigint_handler(int signo) {
  if (foreground_pid != 0) {
    // Enviar SIGINT al proceso en primer plano
    kill(foreground_pid, SIGINT);
  }
  // De lo contrario, ignorar la señal
}

void sigtstp_handler(int signo) {
  if (foreground_pid != 0) {
    // Enviar SIGTSTP al proceso en primer plano
    kill(foreground_pid, SIGTSTP);
  }
  // De lo contrario, ignorar la señal
}

void sigquit_handler(int signo) {
  if (foreground_pid != 0) {
    // Enviar SIGQUIT al proceso en primer plano
    kill(foreground_pid, SIGQUIT);
  }
  // De lo contrario, ignorar la señal
}

void init_shell() {
  animate_startup();
  // Manejo de SIGCHLD como ya tienes
  struct sigaction sa_chld;
  sa_chld.sa_handler = &sigchld_handler;
  /* Se limpia la mascara de seniales, es decir, no se bloquea el resto mientras
   * funciona esta SA_RESTART -> si la señal interrumpe una llamada al sistema
   * (por ejemplo, read), esta debe reiniciarse automáticamente SA_NOCLDSTOP ->
   * Evita que la shell reciba senial SIGCHLD cuando un proceso hijo se detiene,
   * solo se recibe cuando termina
   */
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  /* Si falla el manejador SIGCHLD, se imprime mensaje de error */
  if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
    perror("shell: sigaction");
    exit(EXIT_FAILURE);
  }

  /* Instalar manejadores para SIGINT, SIGTSTP y SIGQUIT
   * Mismo proceso que el SIGCHLD
   */
  struct sigaction sa_int;
  sa_int.sa_handler = &sigint_handler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa_int, NULL) == -1) {
    perror("shell: sigaction SIGINT");
    exit(EXIT_FAILURE);
  }

  struct sigaction sa_tstp;
  sa_tstp.sa_handler = &sigtstp_handler;
  sigemptyset(&sa_tstp.sa_mask);
  sa_tstp.sa_flags = SA_RESTART;

  if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
    perror("shell: sigaction SIGTSTP");
    exit(EXIT_FAILURE);
  }

  struct sigaction sa_quit;
  sa_quit.sa_handler = &sigquit_handler;
  sigemptyset(&sa_quit.sa_mask);
  sa_quit.sa_flags = SA_RESTART;

  if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
    perror("shell: sigaction SIGQUIT");
    exit(EXIT_FAILURE);
  }
}

char *read_command(FILE *input_stream) {
  if (input_stream != stdin) {
    static char input[INPUT_BUFFER_SIZE];
    char *result;

    while (true) {
      result = fgets(input, sizeof(input), input_stream);
      if (result == NULL) {
        if (feof(input_stream)) {
          printf("Reached end of file\n");
          return NULL;
        }
        if (ferror(input_stream)) {
          if (errno == EINTR) {
            clearerr(input_stream);
            continue;
          }
          perror("Error leyendo la entrada");
          return NULL;
        }
      }
      break;
    }

    input[strcspn(input, "\n")] = '\0';
    return input;
  } else {
    // Configuración del prompt
    char hostname[HOST_NAME_MAX];
    char cwd[PATH_MAX];
    char *username;
    struct passwd *pw = getpwuid(getuid());
    username = pw ? pw->pw_name : "unknown";

    if (gethostname(hostname, sizeof(hostname)) == -1) {
      strcpy(hostname, "unknown");
    }
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      strcpy(cwd, "unknown");
    }

    char *current_folder = strrchr(cwd, '/') ? strrchr(cwd, '/') + 1 : cwd;

    char prompt[PROMPT_BUFFER_SIZE];
    snprintf(prompt, sizeof(prompt), "%s%s@%s%s: %s%s%s $ ", COLOR_RED,
             username, hostname, COLOR_RESET, COLOR_WHITE, current_folder,
             COLOR_RESET);
    char *input = readline(prompt);
    if (input && *input) {
      add_history(input);
    }

    return input;
  }
}

int execute_command(char *command) {
  /* Verificar si el comando contiene '|'
   * Si el comando tiene pipes, los divide y ejecuta de forma encadenada
   * Si el comando NO tiene pipes, ejecuta directamente
   */
  if (strchr(command, '|')) {
    // Contiene pipes
    int num_commands = 0;

    /* Divide en subcomandos (tokens)
     * Commands es un array de cadenas, cada token contiene un comando a
     * ejecutar.
     */
    char **commands = split_by_pipes(command, &num_commands);

    /* Ejecuta los comandos de forma encadenada */
    int status = execute_piped_commands(commands, num_commands);

    // Liberar memoria
    for (int i = 0; i < num_commands; i++) {
      free(commands[i]);
    }
    free(commands);

    return status;
  } else {
    // No contiene pipes, ejecutar normalmente
    return execute_single_command(command);
  }
}

/* Funcion para leer y ejecutar comandos desde un archivo */
int execute_batch_file(FILE *batch_file) {
  char *command;

  while ((command = read_command(batch_file)) != NULL) {
    // Ignorar líneas vacías y comentarios
    if (strlen(command) == 0 || command[0] == '#') {
      continue;
    }

    // Ejecutar el comando
    if (execute_command(command) == 0) {
      break; // Salir de la shell si execute_command retorna 0
    }
  }

  return 1;
}

void cleanup_shell() {
  // Limpiar la lista de trabajos en segundo plano
  job_t *current = job_list;
  while (current != NULL) {
    job_t *temp = current;
    current = current->next;
    /* Libero memoria de los nodos y las cadenas asociadas  */
    free(temp->command);
    free(temp);
  }
  job_list = NULL;
}

int add_job(pid_t pid, const char *command) {
  job_t *new_job =
      malloc(sizeof(job_t)); // Asignacion de memoria a nuevo trabajo
  if (!new_job) {
    fprintf(stderr, "Shell: no se pudo asignar memoria para el trabajo.\n");
    return -1;
  }
  new_job->job_id = next_job_id++;    // Incremento ID por cada trabajo nuevo.
  new_job->pid = pid;                 // Almacena la ID del proceso del trabajo
  new_job->command = strdup(command); // Copia el comando proporcionado
  if (!new_job->command) {
    fprintf(stderr, "Shell: no se pudo asignar memoria para el comando.\n");
    free(new_job);
    return -1;
  }
  new_job->next = job_list; // Primer elemento de la lista
  job_list = new_job;       // El puntero job_list apunta al nuevo trabajo
  return new_job->job_id;   // Para rastrear el proceso
}

void remove_job(pid_t pid) {
  /* Busca un trabajo en la lista enlazada usando el pid, eliminando el proceso
   * al encontrarlo y liberando la memoria asignada.
   */
  job_t *current = job_list;
  job_t *prev = NULL;

  while (current != NULL) {
    if (current->pid == pid) {
      if (prev == NULL) {
        job_list = current->next;
      } else {
        prev->next = current->next;
      }
      free(current->command);
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

void sigchld_handler_logic() {
  pid_t pid;
  int status;

  // Esperar a todos los procesos hijos que han terminado
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    remove_job(pid); // Remover el trabajo del proceso terminado
    fflush(stdout);
  }

  if (pid == -1 && errno != ECHILD) {
    perror("waitpid");
  }
}

void animate_startup() {
  printf("%s[BOOT] Initializing kernel...%s\n", COLOR_WHITE, COLOR_RESET);
  usleep(BOOT_INIT_DELAY);

  printf("%s[SYSTEM] Loading legacy modules... 10%% complete%s\n", COLOR_GREEN,
         COLOR_RESET);
  usleep(SYSTEM_LOAD_DELAY);

  printf("%s[NETWORK] Reconnecting to abandoned networks... 40%% complete%s\n",
         COLOR_YELLOW, COLOR_RESET);
  usleep(NETWORK_RECONNECT_DELAY);

  printf("%s[RESOURCE] Gathering power nodes... 75%% complete%s\n", COLOR_BLUE,
         COLOR_RESET);
  usleep(RESOURCE_GATHER_DELAY);

  printf("%s[READY] Shell environment restored successfully.%s\n\n",
         COLOR_MAGENTA, COLOR_RESET);
  usleep(READY_DELAY);

  printf("%s--== Refugee Communications Interface v3.2 ==--%s\n", COLOR_CYAN,
         COLOR_RESET);
  printf("Welcome, survivor. Connectivity status: [%sSTABLE%s]\n", COLOR_GREEN,
         COLOR_RESET);
  printf("Type 'help' for essential commands.\n\n");
  usleep(FINAL_WELCOME_DELAY);
}
