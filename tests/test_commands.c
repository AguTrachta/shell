#include "../include/commands.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MONITOR_PID_FILE "/tmp/monitor_pid"
#define MONITOR_PIPE "/tmp/monitor_pipe"
#define PINK "\033[1;35m"
#define RESET "\033[0m"

/**
 * @brief Test for the 'cd' command functionality.
 *
 * This test verifies the behavior of the 'cd' command in various scenarios:
 * - Changing to a specified directory.
 * - Moving up to the parent directory.
 *
 * It uses assertions to ensure that `cmd_cd` executes successfully
 * and that the current directory is as expected after each change.
 */
void test_cmd_cd()
{
    // Test 1: Change to a specified directory
    mkdir("test_dir", 0755); // Create a directory to test
    char* args1[] = {"cd", "test_dir", NULL};
    int result = cmd_cd(args1);
    assert(result == 1); // Ensure cmd_cd successfully changes directory

    char* current_dir = getcwd(NULL, 0);
    printf("Current directory after 'cd test_dir': %s\n", current_dir);
    assert(strstr(current_dir, "test_dir") != NULL); // Verify we're in "test_dir"
    free(current_dir);

    // Test 2: Change to the parent directory using "cd .."
    char* args2[] = {"cd", "..", NULL};
    result = cmd_cd(args2);
    assert(result == 1); // Check that cmd_cd executes successfully

    current_dir = getcwd(NULL, 0);
    printf("Current directory after 'cd ..': %s\n", current_dir);
    assert(strstr(current_dir, "test_dir") == NULL); // Verify we're no longer in "test_dir"
    free(current_dir);

    // Clean up
    rmdir("test_dir");
    printf("test_cmd_cd passed successfully!\n");
}

/**
 * @brief Test for the 'echo' command functionality.
 *
 * This test validates the `cmd_echo` command by passing a simple message
 * and checking that the function executes successfully.
 */
void test_cmd_echo()
{
    char* args[] = {"echo", "Hello, World!", NULL};
    printf("Expected output: Hello, World!\nActual output: ");
    int result = cmd_echo(args);
    assert(result == 1); // Ensure cmd_echo executes successfully
    printf("test_cmd_echo passed successfully!\n");
}

/**
 * @brief Test for the 'quit' command functionality.
 *
 * This test checks if the `cmd_quit` command returns 0,
 * which indicates that the shell should exit.
 */
void test_cmd_quit()
{
    printf("Testing 'quit' command (should exit with 0):\n");
    int result = cmd_quit();
    assert(result == 0); // Ensure cmd_quit returns 0 to indicate exit
    printf("test_cmd_quit passed successfully!\n");
}

/**
 * @brief Test for starting the monitoring process.
 *
 * This test verifies that the monitoring process is started correctly.
 * It checks if the PID file is created and contains a valid PID.
 */
void test_cmd_start_monitor()
{
    int result = cmd_start_monitor();
    assert(result == 1);

    // Check if PID file was created and contains a valid PID
    FILE* pid_file = fopen(MONITOR_PID_FILE, "r");
    assert(pid_file != NULL);

    pid_t pid;
    assert(fscanf(pid_file, "%d", &pid) == 1); // Check if PID is readable
    fclose(pid_file);

    // Check if process with that PID exists
    assert(kill(pid, 0) == 0); // Process should exist

    printf("test_cmd_start_monitor passed successfully!\n");
}

/**
 * @brief Test for stopping the monitoring process.
 *
 * This test starts the monitoring process, then stops it.
 * It verifies that the PID file is deleted and that the process is no longer running.
 */
void test_cmd_stop_monitor()
{
    // Start the monitoring process
    cmd_start_monitor();

    // Read the PID from the file
    FILE* pid_file = fopen(MONITOR_PID_FILE, "r");
    assert(pid_file != NULL);

    pid_t pid;
    assert(fscanf(pid_file, "%d", &pid) == 1);
    fclose(pid_file);

    // Stop the monitoring process
    int result = cmd_stop_monitor();
    assert(result == 1);

    // Check if PID file was deleted
    pid_file = fopen(MONITOR_PID_FILE, "r");
    assert(pid_file == NULL); // PID file should not exist

    // Verify that the process is no longer running
    assert(kill(pid, 0) == -1); // Should fail, as the process should not exist

    printf("test_cmd_stop_monitor passed successfully!\n");
}

/**
 * @brief Test for executing piped commands.
 *
 * This test creates a temporary file with sample content, then tests
 * a piped command (`cat file | grep "Hello"`) to verify the output.
 *
 * It captures the output of the piped command and checks that it
 * contains the expected lines.
 */
void test_piped_commands()
{
    // Create a temporary file with sample content
    const char* temp_filename = "temp_test_file.txt";
    FILE* temp_file = fopen(temp_filename, "w");
    if (temp_file == NULL)
    {
        perror("Error creating temp file for piped command test");
        return;
    }

    // Write some sample content to the file
    fprintf(temp_file, "Hello World\nThis is a test file\nTesting grep functionality\nHello again\n");
    fclose(temp_file);

    // Define piped commands: `cat temp_test_file.txt` and `grep "Hello"`
    char* commands[] = {"cat temp_test_file.txt", "grep \"Hello\""};

    // Redirect stdout to capture the output of the piped command
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Pipe creation failed");
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }

    if (pid == 0)
    {
        // Child process: Redirect stdout to the write end of the pipe
        close(pipe_fd[0]); // Close unused read end in child
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2 failed");
            close(pipe_fd[1]);
            exit(1);
        }
        close(pipe_fd[1]); // Close the original write end after duplicating

        // Execute the piped commands
        execute_piped_commands(commands, 2);

        exit(0); // Exit after executing commands
    }
    else
    {
        // Parent process: Capture the output from the read end of the pipe
        close(pipe_fd[1]); // Close unused write end in parent

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0'; // Null-terminate the buffer

            // Verify the output contains the expected lines
            if (strstr(buffer, "Hello World") != NULL && strstr(buffer, "Hello again") != NULL)
            {
                printf("Piped command output:\n%s\n", buffer);
                printf("test_piped_commands passed successfully!\n");
            }
            else
            {
                fprintf(stderr, "Unexpected output from piped command\n");
            }
        }
        else if (bytes_read == -1)
        {
            perror("Failed to read output from pipe");
        }

        close(pipe_fd[0]); // Close the read end after use
        wait(NULL);        // Wait for the child process to finish
    }

    // Delete the temporary file after using it
    if (unlink(temp_filename) == -1)
    {
        perror("Failed to delete temporary file");
    }
}

/**
 * @brief Main function to run all tests.
 *
 * This function calls each test function in sequence,
 * printing a message before each test starts and asserting that all tests pass.
 *
 * @return 0 upon successful completion of all tests.
 */
int main()
{
    printf(PINK "\n\n==== Running test: test_cmd_cd ====\n" RESET);
    test_cmd_cd();

    printf(PINK "\n\n==== Running test: test_cmd_echo ====\n" RESET);
    test_cmd_echo();

    printf(PINK "\n\n==== Running test: test_cmd_quit ====\n" RESET);
    test_cmd_quit();

    printf(PINK "\n\n==== Running test: test_cmd_start_monitor ====\n" RESET);
    test_cmd_start_monitor();

    printf(PINK "\n\n==== Running test: test_cmd_stop_monitor ====\n" RESET);
    test_cmd_stop_monitor();

    printf(PINK "\n\n==== Running test: test_piped_commands ====\n" RESET);
    test_piped_commands();

    return 0;
}
