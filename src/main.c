#include "commands.h"
#include "shell.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    // Inicializar la shell
    init_shell();

    // Si se proporciona un batch file, ejecutarlo
    if (argc == 2)
    {
        FILE* batch_file = fopen(argv[1], "r");
        if (!batch_file)
        {
            perror("Error opening batch file");
            exit(EXIT_FAILURE);
        }

        // Ejecutar comandos desde el batch file
        execute_batch_file(batch_file);
        fclose(batch_file);
        cleanup_shell();
        return 0;
    }
    // Error de uso, mas de un argumento.
    else if (argc > 2)
    {
        fprintf(stderr, "Uso: %s [archivo_de_comandos]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Modo interactivo
    while (true)
    {
        if (sigchld_flag)
        {
            sigchld_flag = 0;
            // Lógica para manejar SIGCHLD
            sigchld_handler_logic();
        }

        char* input = read_command(stdin);
        if (input == NULL)
        {
            // Manejar EOF (Ctrl+D)
            printf("\nGetting out of shell.\n");
            free(input);
            break;
        }

        // Ejecutar el comando ingresado
        if (execute_command(input) == 0)
        {
            free(input);
            break; // Salir de la shell si execute_command retorna 0
        }
    }
    // Limpiar recursos antes de salir
    cleanup_shell();
    return 0;
}
