#include "commands.h"
#include "parse.h"
#include "shell.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096 // General buffer size for reading data
#define JSON_ACCUMULATED_BUFFER_SIZE                                           \
  (BUFFER_SIZE * 2) // Buffer size for accumulated JSON data

#define MONITOR_PID_FILE "/tmp/monitor_pid"
#define MONITOR_PIPE "/tmp/monitor_pipe"
#define MAX_READ_ATTEMPTS 5 // Maximum number of read attempts
#define PID_WAIT_TIME                                                          \
  1 // Sleep time in seconds to wait for the process to terminate

#define CLEAR_SCREEN_CODE "\033[H\033[J" // ANSI escape code to clear screen
pid_t monitor_pid = 0;

int cmd_searchconfig(char **args) {
  // Check if the user provided a directory
  if (args[1] == NULL) {
    printf("Usage: searchconfig <directory> [extension]\n");
    return 1;
  }

  const char *directory = args[1];
  // Default extension is ".config" if not provided
  const char *extension = args[2] ? args[2] : ".config";

  printf("Exploring directory: %s for '%s' files\n", directory, extension);

  search_directory_recursive(directory, extension);

  return 1; // Continue the shell
}

// Implement the recursive directory traversal function
void search_directory_recursive(const char *directory, const char *extension) {
  DIR *dir = opendir(directory);
  if (!dir) {
    perror(directory);
    return;
  }

  struct dirent *entry;
  char path[PATH_MAX]; // Use PATH_MAX for maximum path length

  while ((entry = readdir(dir)) != NULL) {
    // Skip '.' and '..' directories
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Build the full path
    snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
      perror(path);
      continue;
    }

    if (S_ISDIR(statbuf.st_mode)) {
      // Recursively search subdirectories
      search_directory_recursive(path, extension);
    } else if (S_ISREG(statbuf.st_mode)) {
      // Check if the file has the desired extension
      if (has_extension(entry->d_name, extension)) {
        printf("\nConfiguration file found: %s\n", path);
        print_file_content(path);
      }
    }
  }

  closedir(dir);
}

// Helper function to check if a filename has the given extension
int has_extension(const char *filename, const char *extension) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return 0;
  return strcmp(dot, extension) == 0;
}

// Helper function to print the content of a file
void print_file_content(const char *filepath) {
  printf("Content of %s:\n", filepath);

  FILE *file = fopen(filepath, "r");
  if (!file) {
    perror(filepath);
    return;
  }

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    printf("%s", line);
  }

  fclose(file);
}

int cmd_start_monitor() {
  // Verificar si ya hay un proceso de monitoreo activo leyendo el archivo de
  // PID
  FILE *pid_file = fopen(MONITOR_PID_FILE, "r");
  if (pid_file) {
    pid_t existing_pid;
    if (fscanf(pid_file, "%d", &existing_pid) == 1) {
      // Si el proceso está activo, detenerlo antes de iniciar uno nuevo
      if (kill(existing_pid, 0) == 0) {
        printf("Stopping existing monitoring process (PID: %d)\n",
               existing_pid);
        kill(existing_pid, SIGTERM);
        sleep(PID_WAIT_TIME); // Esperar un momento para asegurar que el proceso
                              // termine
      }
    }
    fclose(pid_file);
  }

  // Iniciar un nuevo proceso de monitoreo
  pid_t pid = fork();
  if (pid < 0) {
    perror("Error starting the monitoring process");
    return 1;
  } else if (pid == 0) {
    // Proceso hijo: redirigir stdout y stderr a /dev/null para evitar mostrar
    // en pantalla
    int fd_null = open("/dev/null", O_WRONLY);
    if (fd_null == -1) {
      perror("Error opening /dev/null");
      exit(EXIT_FAILURE);
    }
    dup2(fd_null, STDOUT_FILENO);
    dup2(fd_null, STDERR_FILENO);
    close(fd_null);

    // Ejecutar el monitor en la ruta especificada
    execlp("./monitoring_project", "monitoring_project", NULL);

    // Si execlp falla
    perror("Error executing the monitoring program");
    exit(EXIT_FAILURE);
  } else {
    // Proceso padre: guardar el PID en el archivo y mostrar mensaje
    monitor_pid = pid;
    pid_file = fopen(MONITOR_PID_FILE, "w");
    if (pid_file) {
      fprintf(pid_file, "%d\n", monitor_pid);
      fclose(pid_file);
    }
    printf("Monitoring process started with PID: %d\n", monitor_pid);
  }
  return 1;
}

int cmd_stop_monitor() {
  // Open the PID file to read the monitoring process PID
  FILE *pid_file = fopen(MONITOR_PID_FILE, "r");
  if (!pid_file) {
    perror("Monitor: Could not open PID file. Is the monitor running?");
    return 1;
  }

  pid_t existing_pid;
  if (fscanf(pid_file, "%d", &existing_pid) != 1) {
    fprintf(stderr, "Monitor: Could not read PID from file.\n");
    fclose(pid_file);
    return 1;
  }
  fclose(pid_file);

  // Send SIGKILL to stop the monitoring process directly
  if (kill(existing_pid, SIGKILL) == -1) {
    perror("Monitor: Error sending SIGKILL to monitoring process");
    return 1;
  }

  // Delete the PID file after stopping the process
  if (remove(MONITOR_PID_FILE) != 0) {
    perror("Monitor: Error deleting PID file");
    // Not critical, so we don't return an error
  } else {
    printf("Monitor: PID file deleted successfully.\n");
  }

  return 1; // Indicate to the shell to keep running
}

int cmd_status_monitor(const char *option) {
  if (strcmp(option, "--help") == 0) {
    printf("\n--- Help for status_monitor command ---\n");
    printf("Usage: status_monitor [options]\n");
    printf("Options:\n");
    printf("  -c     Shows only CPU usage\n");
    printf("  -m     Shows only memory usage\n");
    printf("  -d     Shows only disk statistics (reads, writes, time)\n");
    printf(
        "  -n     Shows only network statistics (bandwidth, packet ratio)\n");
    printf("  -p     Shows only the count of running processes\n");
    printf("  -s     Shows only context switches\n");
    printf("No option: Shows all system metrics\n");
    printf("-------------------------------------------\n\n");
    return 1;
  }

  int fd = open(MONITOR_PIPE, O_RDONLY);
  if (fd == -1) {
    perror("Error opening pipe to read metrics");
    return 1;
  }

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  cJSON *root = NULL;

  while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';

    // Identify the last complete JSON object in the buffer
    char *start = strrchr(buffer, '{');
    char *end = strrchr(buffer, '}');

    if (start && end && end > start) {
      size_t json_length = end - start + 1;
      char json_object[json_length + 1];
      strncpy(json_object, start, json_length);
      json_object[json_length] = '\0';

      root = cJSON_Parse(json_object);
      if (!root) {
        fprintf(stderr, "Error parsing JSON or incomplete JSON: %s\n",
                json_object);
        continue;
      }

      printf("\n------ Monitoring System ------\n");

      if (strcmp(option, "-c") == 0) // CPU
      {
        cJSON *item = cJSON_GetObjectItem(root, "cpu_usage_percentage");
        printf("CPU Usage: %.2f%%\n", item ? item->valuedouble : 0.0);
      } else if (strcmp(option, "-m") == 0) // Memory
      {
        cJSON *item = cJSON_GetObjectItem(root, "memory_usage_percentage");
        printf("Memory Usage: %.2f%%\n", item ? item->valuedouble : 0.0);
      } else if (strcmp(option, "-d") == 0) // Disk
      {
        cJSON *item = cJSON_GetObjectItem(root, "disk_reads");
        printf("Disk Reads: %.0f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "disk_writes");
        printf("Disk Writes: %.0f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "disk_read_time_seconds");
        printf("Disk Read Time (s): %.2f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "disk_write_time_seconds");
        printf("Disk Write Time (s): %.2f\n", item ? item->valuedouble : 0.0);
      } else if (strcmp(option, "-n") == 0) // Network
      {
        cJSON *item = cJSON_GetObjectItem(root, "network_bandwidth_rx");
        printf("Network RX (bytes): %.0f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "network_bandwidth_tx");
        printf("Network TX (bytes): %.0f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "network_packet_ratio");
        printf("Packet Ratio: %.2f\n", item ? item->valuedouble : 0.0);
      } else if (strcmp(option, "-p") == 0) // Processes and Context Switches
      {
        cJSON *item = cJSON_GetObjectItem(root, "running_processes_count");
        printf("Running Processes: %.0f\n", item ? item->valuedouble : 0.0);
        item = cJSON_GetObjectItem(root, "context_switches_total");
        printf("Context Switches: %.0f\n", item ? item->valuedouble : 0.0);
      } else // Show all metrics
      {
        printf("CPU Usage: %.2f%%\n",
               cJSON_GetObjectItem(root, "cpu_usage_percentage")->valuedouble);
        printf(
            "Memory Usage: %.2f%%\n",
            cJSON_GetObjectItem(root, "memory_usage_percentage")->valuedouble);
        printf("Disk Reads: %.0f\n",
               cJSON_GetObjectItem(root, "disk_reads")->valuedouble);
        printf("Disk Writes: %.0f\n",
               cJSON_GetObjectItem(root, "disk_writes")->valuedouble);
        printf(
            "Disk Read Time (s): %.2f\n",
            cJSON_GetObjectItem(root, "disk_read_time_seconds")->valuedouble);
        printf(
            "Disk Write Time (s): %.2f\n",
            cJSON_GetObjectItem(root, "disk_write_time_seconds")->valuedouble);
        printf("Network RX (bytes): %.0f\n",
               cJSON_GetObjectItem(root, "network_bandwidth_rx")->valuedouble);
        printf("Network TX (bytes): %.0f\n",
               cJSON_GetObjectItem(root, "network_bandwidth_tx")->valuedouble);
        printf("Packet Ratio: %.2f\n",
               cJSON_GetObjectItem(root, "network_packet_ratio")->valuedouble);
        printf(
            "Running Processes: %.0f\n",
            cJSON_GetObjectItem(root, "running_processes_count")->valuedouble);
        printf(
            "Context Switches: %.0f\n",
            cJSON_GetObjectItem(root, "context_switches_total")->valuedouble);
      }

      printf("----------------------------------\n");

      cJSON_Delete(root);
    }
  }

  if (bytes_read == -1) {
    perror("Error reading from pipe");
  }

  close(fd);
  return 1;
}

int cmd_help() {
  printf("\n--- List of Internal Commands ---\n");
  printf("cd [dir]           - Changes the current directory.\n");
  printf("clear              - Clears the screen.\n");
  printf("echo [text]        - Displays text or environment variables.\n");
  printf("quit               - Exits the shell.\n");
  printf("start_monitor      - Starts the monitoring process.\n");
  printf("stop_monitor       - Stops the monitoring process.\n");
  printf("status_monitor     - Displays the system monitoring status.\n");
  printf("searchconfig <directory> [extension] - Searches for configuration "
         "files.\n");
  printf("help               - Shows this list of internal commands.\n");

  printf("\n--- External Commands ---\n");
  printf("Any external command available on the system, such as 'ls', 'cat', "
         "'grep', etc.\n");
  printf("For more details on external commands, use 'man [command]'.\n");

  printf("\n--- Using Pipes (|) ---\n");
  printf("Use the '|' operator to chain commands, allowing the output of one "
         "command to be the input of another.\n");
  printf("Example: ls | grep 'name'\n");

  printf("\n--- Input and Output Redirection ---\n");
  printf("Use '>' to redirect the output of a command to a file, and '<' to "
         "read from a file as input.\n");
  printf("Examples:\n");
  printf("  echo 'Hello' > file.txt    - Writes 'Hello' to file.txt.\n");
  printf("  cat < file.txt             - Reads and displays the contents of "
         "file.txt.\n");

  printf("\n--- Executing Command Scripts ---\n");
  printf("You can execute files with a series of commands by using: ./filename "
         "or sh filename.\n");
  printf("Additionally, you can pass a command file when starting the shell to "
         "execute it automatically:\n");
  printf("  Usage: ./shell [command_file]\n");

  printf("-----------------------------------------\n\n");
  return 1;
}

int cmd_cd(char **args) // args[1] es el directorio al que se quiere cambiar
{
  char *target_dir;  // Directorio al que se quiere cambiar
  char *current_dir; // Guarda directorio actual antes de cambiarlo

  // Soportar 'cd ..' para volver al directorio padre
  if (args[1] == NULL || strcmp(args[1], "..") == 0) {
    target_dir = "..";
  } else {
    target_dir = args[1];
  }

  // Obtener el directorio actual antes de cambiar
  current_dir = getcwd(NULL, 0);
  if (current_dir == NULL) {
    perror("cd: getcwd failed");
    return 1;
  }

  // Cambiar el directorio
  if (chdir(target_dir) !=
      0) // Cambia el directorio del proceso actual al que voy a pasar.
  {
    perror("cd");
    free(current_dir);
    return 1;
  }

  // Actualizar OLDPWD
  setenv("OLDPWD", current_dir, 1);
  free(current_dir);

  // Actualizar PWD
  current_dir = getcwd(NULL, 0);
  if (current_dir == NULL) {
    perror("cd: getcwd falló");
    return 1;
  }
  setenv("PWD", current_dir, 1);
  free(current_dir);

  return 1;
}

int cmd_clr() {
  // Usar ANSI escape codes para limpiar la pantalla de manera más eficiente
  printf(CLEAR_SCREEN_CODE);
  return 1;
}

int cmd_echo(char **args) {
  if (args[1] == NULL) // No se pasa ningun argumento despues de echo
  {
    printf("\n");
    return 1;
  }

  /* Interpreta las variables de entorno e imprime todos los argumentos*/
  for (int i = 1; args[i] != NULL; i++) {
    if (args[i][0] == '$') {
      // Expandir variables de entorno
      char *env_var = getenv(
          args[i] +
          1); // Si el argumento es variable de entorno se imprime el contenido
      if (env_var != NULL) {
        printf("%s ", env_var);
      } else {
        printf("$%s ", args[i] + 1);
      }
    } else {
      printf("%s ", args[i]);
    }
  }
  printf("\n");
  return 1;
}

int cmd_quit() {
  // Puedes realizar limpieza adicional aquí si es necesario
  printf("Getting out from shell.\n");
  return 0; // Cuando retorna a 0 significa que sale de la shell
}

int execute_internal_command(char **args, char *input_file, char *output_file,
                             int background) {
  if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "clear") == 0 ||
      strcmp(args[0], "echo") == 0 || strcmp(args[0], "quit") == 0 ||
      strcmp(args[0], "help") == 0 || strcmp(args[0], "start_monitor") == 0 ||
      strcmp(args[0], "stop_monitor") == 0 ||
      strcmp(args[0], "status_monitor") == 0 ||
      strcmp(args[0], "searchconfig") == 0) // Verifica si el comando es interno
  {
    int saved_stdin = -1,
        saved_stdout = -1;       // Descriptores de archivos originales
    int fd_in = -1, fd_out = -1; // Descriptores de archivos (si se especifican)

    /* Manejar redirección de entrada
     * El uso de dup y dup2 me permiten redirigir STDIN o STDOUT a sus
     * respectivos archivos para la ejecución de un código como echo EJ: echo
     * "Hello, world!" > output.txt */
    if (input_file) {
      fd_in = open(input_file, O_RDONLY); // Leer archivo
      if (fd_in == -1) {
        perror("Shell: error al abrir archivo de entrada");
        return 1;
      }
      saved_stdin = dup(STDIN_FILENO); // Se duplica para restauración
      if (dup2(fd_in, STDIN_FILENO) ==
          -1) // Permite modificar el archivo temporalmente sin tener que
              // cambiar el original
      {
        perror("Shell: dup2 input");
        close(fd_in);
        return 1;
      }
      close(fd_in); // Ya no se usa, leo desde STDIN
    }

    // Manejar redirección de salida, lo mismo que lo anterior
    if (output_file) {
      fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      /* O_CREAT -> Crea archivo si no existe
       * O_TRUNC -> Vacía el contenido del archivo si este existe
       */
      if (fd_out == -1) {
        perror("Shell: error al abrir archivo de salida");
        if (saved_stdin != -1) {
          dup2(saved_stdin, STDIN_FILENO);
          close(saved_stdin);
        }
        return 1;
      }
      saved_stdout = dup(STDOUT_FILENO);
      if (dup2(fd_out, STDOUT_FILENO) == -1) {
        perror("Shell: dup2 output");
        close(fd_out);
        if (saved_stdin != -1) {
          dup2(saved_stdin, STDIN_FILENO);
          close(saved_stdin);
        }
        return 1;
      }
      close(fd_out);
    }

    // Ejecutar el comando interno
    int result = -1;
    if (strcmp(args[0], "cd") == 0) {
      result = cmd_cd(args);
    } else if (strcmp(args[0], "clear") == 0) {
      result = cmd_clr();
    } else if (strcmp(args[0], "help") == 0) {
      result = cmd_help();
    } else if (strcmp(args[0], "searchconfig") == 0) {
      result = cmd_searchconfig(args);
    } else if (strcmp(args[0], "echo") == 0) {
      if (background) {
        pid_t pid = fork();
        if (pid < 0) {
          // Error en fork
          perror("Shell: fork");
          result = 1;
        } else if (pid == 0) {
          // Proceso hijo

          /* Restaurar manejadores de señales a los valores por defecto
           * Esto es necesario para que los comandos internos puedan responder
           * correctamente a señales como SIGINT, SIGTSTP, etc.*/
          signal(SIGINT, SIG_DFL);
          signal(SIGTSTP, SIG_DFL);
          signal(SIGQUIT, SIG_DFL);

          // Ejecutar el comando 'echo'
          result = cmd_echo(args);

          // Restaurar los descriptores originales si STDIN o STDOUT fueron
          // usados.
          if (saved_stdin != -1) {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
          }
          if (saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
          }

          // Terminar el proceso hijo
          exit(EXIT_SUCCESS);
        } else {
          // Proceso padre

          // Agregar el trabajo en segundo plano
          int job_id = add_job(pid, "echo");
          if (job_id !=
              -1) // Se agregó el trabajo en segundo plano exitosamente
          {
            printf("[%d] %d\n", job_id, pid);
          } else {
            // Si no se pudo agregar el trabajo, esperar al proceso
            waitpid(pid, NULL, 0);
          }
          result = 1;
        }
      } else {
        result = cmd_echo(args);
      }
    } else if (strcmp(args[0], "quit") == 0) {
      result = cmd_quit();
    } else if (strcmp(args[0], "start_monitor") == 0) {
      result = cmd_start_monitor();
    } else if (strcmp(args[0], "stop_monitor") == 0) {
      result = cmd_stop_monitor();
    } else if (strcmp(args[0], "status_monitor") == 0) {
      // Manejar opción adicional para status_monitor
      const char *option =
          args[1] ? args[1] : ""; // Obtiene el argumento, si existe
      result = cmd_status_monitor(option);
    }

    // Restaurar los descriptores originales si STDIN o STDOUT fueron usados.
    if (!background) // Ya se restauraron en el proceso hijo si es en segundo
                     // plano
    {
      if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
      }
      if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
      }
    }

    return result;
  }

  return -1; // No es un comando interno
}

int execute_external_command(char **args, int background, char *command_copy,
                             char *input_file, char *output_file) {
  pid_t pid;
  int status;

  pid = fork(); // Proceso hijo.
  if (pid < 0) {
    // Error en fork
    perror("Shell: fork");
    return 1;
  } else if (pid == 0) {
    // Proceso hijo

    /* Restaurar manejadores de señales a los valores por defecto
     * Esto es necesario para que los comandos externos puedan responder
     * correctamente a CTRL-C, CTRL-Z, etc*/
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    // Manejar redirección de entrada, explicado en el anterior bloque
    if (input_file) {
      int fd_in = open(input_file, O_RDONLY);
      if (fd_in == -1) {
        perror("Shell: error al abrir archivo de entrada");
        exit(EXIT_FAILURE);
      }
      if (dup2(fd_in, STDIN_FILENO) == -1) {
        perror("Shell: dup2 input");
        close(fd_in);
        exit(EXIT_FAILURE);
      }
      close(fd_in);
    }

    // Manejar redirección de salida
    if (output_file) {
      int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_out == -1) {
        perror("Shell: error al abrir archivo de salida");
        exit(EXIT_FAILURE);
      }
      if (dup2(fd_out, STDOUT_FILENO) == -1) {
        perror("Shell: dup2 output");
        close(fd_out);
        exit(EXIT_FAILURE);
      }
      close(fd_out);
    }

    // Ejecutar el comando, execv y execvp reemplazan el proceso actual por el
    // programa especificado execv usa el comando con el directorio
    // especificado, por ej: /usr/bin/ls execvp usa el comando buscando el
    // directorio en PATH, por ej: ls
    if (strchr(args[0], '/')) {
      if (execv(args[0], args) == -1) {
        perror("Shell");
        exit(EXIT_FAILURE);
      }
    } else {
      if (execvp(args[0], args) == -1) {
        perror("Shell");
        exit(EXIT_FAILURE);
      }
    }
  } else // Proceso padre
  {
    if (background) {
      // Proceso padre, ejecución en segundo plano
      int job_id = add_job(pid, command_copy);
      if (job_id != -1) // Se agrego el trabajo en segundo plano exitosamente
      {
        printf("[%d] %d\n", job_id, pid);
      } else {
        // Si no se pudo agregar el trabajo, esperar al proceso
        waitpid(pid, NULL, 0);
      }
    } else {
      // Proceso padre, ejecución en primer plano

      // Establecer foreground_pid, indicando que es el proceso en primer plano
      foreground_pid = pid;

      do {
        pid_t wpid = waitpid(pid, &status, WUNTRACED); // Espera al proceso hijo
        if (wpid == -1) {
          /* Si el error es por interrupcion del sistema, como read, write, otro
           * waitpid implica que esa senial interrumpio la operacion antes de
           * que se complete, y el sistema puede asumir que el proceso hijo ya
           * termino cuando no es asi, por eso se continua esperando
           * */
          if (errno == EINTR) {
            continue;
          } else {
            perror("Shell: waitpid");
            break;
          }
        }
        if (WIFSTOPPED(status)) {
          // El proceso fue detenido (CTRL-Z)
          printf("\nProcess %d detained\n", pid);
          break;
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));

      // Proceso en primer plano terminó o fue detenido
      foreground_pid = 0;
    }
  }
  return 1;
}

int execute_single_command(char *command) {
  char *input_file = NULL;
  char *output_file = NULL;
  char **args = parse_command(command, &input_file, &output_file);

  if (args[0] == NULL) {
    // Comando vacío
    free(args);
    if (input_file) {
      free(input_file);
    }
    if (output_file) {
      free(output_file);
    }
    return 1;
  }

  // Verificar si el comando debe ejecutarse en segundo plano
  int background = 0;
  int i = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], "&") == 0) {
      background = 1;
      args[i] = NULL; // Eliminar '&' de los argumentos
      break;
    }
    i++;
  }

  // Verificar si es un comando interno
  int internal_status =
      execute_internal_command(args, input_file, output_file, background);
  if (internal_status != -1) {
    // Es un comando interno
    free(args);
    if (input_file) {
      free(input_file);
    }
    if (output_file) {
      free(output_file);
    }
    return internal_status;
  }

  // Ejecutar como comando externo
  int status = execute_external_command(args, background, command, input_file,
                                        output_file);

  free(args);
  if (input_file) {
    free(input_file);
  }
  if (output_file) {
    free(output_file);
  }
  return status;
}

int execute_piped_commands(char **commands, int num_commands) {
  int i;
  int in_fd = STDIN_FILENO; // Entrada inicial
  pid_t pid;
  int status;
  int fd[2]; // Array para los descriptores de archivos (f[0] leer, f[1]
             // escribir)

  pid_t *pids = malloc(num_commands * sizeof(pid_t)); // Varios procesos hijos
  if (!pids) {
    fprintf(stderr, "Shell: error de asignación de memoria\n");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < num_commands; i++) // Crea varios procesos hijos
  {
    // Crear el pipe para este comando, excepto en el último
    if (i < num_commands - 1) {
      /* Se crea pipe con pipe(fd) que conecta la salida del comando actual con
       * la entrada del siguiente */
      if (pipe(fd) == -1) {
        perror("Shell: pipe");
        exit(EXIT_FAILURE);
      }
    } else {
      fd[0] = STDIN_FILENO;
      fd[1] = STDOUT_FILENO;
    }

    pid = fork();
    if (pid == -1) {
      perror("Shell: fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      // Proceso hijo

      // Restaurar manejadores de señales
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGQUIT, SIG_DFL);

      // Redireccionar entrada si no es el primer comando
      if (in_fd != STDIN_FILENO) {
        if (dup2(in_fd, STDIN_FILENO) == -1) {
          perror("Shell: dup2 in_fd");
          exit(EXIT_FAILURE);
        }
        close(in_fd);
      }

      // Redireccionar salida si no es el último comando
      if (i < num_commands - 1) {
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
          perror("Shell: dup2 fd[1]");
          exit(EXIT_FAILURE);
        }
        close(fd[1]);
        close(fd[0]);
      }

      // Parsear el comando actual en argumentos, incluyendo redirección
      char *input_file = NULL;
      char *output_file = NULL;
      char **args = parse_command(commands[i], &input_file, &output_file);

      if (args[0] == NULL) {
        // Comando vacío
        free(args);
        if (input_file)
          free(input_file);
        if (output_file)
          free(output_file);
        exit(EXIT_FAILURE);
      }

      // Manejar redirección de entrada
      if (input_file) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in == -1) {
          perror("Shell: error al abrir archivo de entrada");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {
          perror("Shell: dup2 input");
          close(fd_in);
          exit(EXIT_FAILURE);
        }
        close(fd_in);
        free(input_file);
      }

      // Manejar redirección de salida
      if (output_file) {
        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
          perror("Shell: error al abrir archivo de salida");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
          perror("Shell: dup2 output");
          close(fd_out);
          exit(EXIT_FAILURE);
        }
        close(fd_out);
        free(output_file);
      }

      // Ejecutar el comando
      if (execvp(args[0], args) == -1) {
        perror("Shell");
        // Liberar memoria antes de salir
        int j = 0;
        while (args[j]) {
          free(args[j]);
          j++;
        }
        free(args);
        exit(EXIT_FAILURE);
      }
    } else {
      // Proceso padre

      pids[i] = pid;

      // Cerrar descriptores que no se necesitan
      if (in_fd != STDIN_FILENO) {
        close(in_fd);
      }
      if (i < num_commands - 1) {
        close(fd[1]);
      }

      // Preparar la entrada para el siguiente comando
      in_fd = fd[0];
    }
  }

  // Esperar a todos los procesos hijos
  for (i = 0; i < num_commands; i++) {
    do {
      pid_t wpid = waitpid(pids[i], &status, 0);
      if (wpid == -1) {
        if (errno == EINTR) {
          continue;
        } else {
          perror("Shell: waitpid");
          break;
        }
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  free(pids);
  return 1;
}
