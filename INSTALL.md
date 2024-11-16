# INSTALL.md

## Setup Instructions

This document will guide you through the process of setting up, compiling, running, and testing the shell and monitoring program.

### Prerequisites

1. **Python Environment**: Ensure you have Python installed on your system to create a virtual environment.
2. **Conan**: Required to manage dependencies.
3. **CMake**: Required for building the project.
4. **Make**: To compile the generated build files.
5. **Docker** (optional): To simplify the setup and ensure a consistent environment.

### Steps

1. **Set up a Python Virtual Environment**:

   - In the root directory of the shell project, create a Python virtual environment:
     ```bash
     python3 -m venv myenv
     ```
   - Activate the environment:
     ```bash
     source myenv/bin/activate
     ```
   - Install Conan inside the virtual environment:
     ```bash
     pip install conan
     ```

2. **Prepare the Build Directory**:

   - Create and navigate to the `build` directory:
     ```bash
     mkdir build
     cd build
     ```

3. **Install Dependencies with Conan**:

   - Ensure the virtual environment (`myenv`) is activated before running Conan.
   - Install dependencies (you can deactivate the virtual environment afterward):
     ```bash
     conan install .. --build=missing
     ```
   - Deactivate the virtual environment:
     ```bash
     deactivate
     ```

4. **Generate Build Files with CMake**:

   - From within the `build` directory, generate the build files:
     ```bash
     cmake -DCMAKE_BUILD_TYPE=Release ..
     ```

5. **Compile the Project**:

   - Run `make` to compile the project:
     ```bash
     make
     ```

   - This will generate three executable files in the `build` directory:
     - `shell`: The main shell executable
     - `monitoring_program`: The monitoring program executable
     - `test_command`: Executable for running tests

### Running the Shell Program

- To start the shell, simply run:
  ```bash
  ./shell
  ```

### Running the Monitoring Program

- The monitoring program is linked within the shell's functionality. If you need to run it independently, execute:
  ```bash
  ./monitoring_program
  ```

### Testing and Coverage

- **Run Tests**:

  - Execute the tests using `test_command`:
    ```bash
    ./test_command
    ```

- **Generate Coverage Report**:

  - Run the `run_coverage.sh` script to generate a coverage report:
    ```bash
    ./run_coverage.sh
    ```

  - The coverage report will be generated in the `coverage_report/` directory.

- **View Coverage Report**:

  Open the HTML coverage report (if you’re on a Linux system with xdg-open installed):
  ```bash
  xdg-open coverage_report/index.html
  ```

### Using Docker

Alternatively, you can use Docker to simplify the setup process:

#### Prerequisites for Docker

- **Docker**: Ensure Docker is installed on your system. You can download and install it from [Docker's official site](https://www.docker.com/products/docker-desktop).

#### Using Docker for Setup

To simplify the installation process and avoid dependency issues, you can use Docker. The following steps will help you build and run your shell project in a Docker container:

1. **Build the Docker Image**:

   In the root directory of your project (where the Dockerfile is located), build the Docker image by running:

   ```bash
   docker build -t my_shell_image .
   ```

   This command creates a Docker image called `my_shell_image` that contains everything needed to compile and run the shell.

2. **Run the Docker Container**:

   To run the shell in a Docker container:

   ```bash
   docker run -it --rm my_shell_image
   ```

   Explanation:
   - `-it`: Runs the container in interactive mode with a TTY so you can interact with the shell.
   - `--rm`: Automatically removes the container once you exit, helping keep your system clean.

   After running this command, you will be dropped into an interactive terminal inside the Docker container. From here, you can compile and run the project as you would normally.

3. **Run Executables Inside the Docker Container**:

   - **Run the Shell Program**:
     ```bash
     ./shell
     ```

   - **Run the Monitoring Program**:
     ```bash
     ./monitoring_program
     ```

   - **Run Tests**:
     ```bash
     ./test_command
     ```

4. **Generate and View Coverage Report**:

   - **Generate Coverage Report**:
     ```bash
     ./run_coverage.sh
     ```
     This command will generate a code coverage report in the `coverage_report/` directory.

   - **View Coverage Report**:
     If your system supports `xdg-open`, you can open the coverage report by running:
     ```bash
     xdg-open coverage_report/index.html
     ```

#### Why Use Docker?

Using Docker provides a consistent and isolated environment, which helps to:
- **Avoid Dependency Issues**: Docker ensures all dependencies are set up correctly, without requiring manual installation.
- **Ensure Consistency**: The environment is the same across all systems, making it easier for different developers to work on the same project without compatibility issues.

### Summary

This guide provides detailed steps to set up the shell project, either by manually configuring your system or using Docker for a simpler and more consistent environment. Docker instructions have been expanded to help new users easily get started. If you have any questions or issues, please consult the Docker documentation or reach out for support.


