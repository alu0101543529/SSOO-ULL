/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Sistemas Operativos 2023-2024
 *
 * @author Raúl González Acosta (alu0101543529@ull.edu.es)
 * @date   30/10/2023
 * @brief  Implementación de la clase Subprocess
*/

#include "header_files/subprocess.h"

/**
 * @brief Constructor de subprocess
 * @param[in] args: representa los argumentos del comando a ejecutar
 * @param redirected_io: miembro de la enumeración `stdio` que indica cómo se manejará la entrada/salida estándar del proceso hijo.
 */
subprocess::subprocess(const std::vector<std::string>& args, subprocess::stdio redirected_io) : args(args), redirected_io(redirected_io), child_pid(-1) {
  pipe(stdin_pipe);
  pipe(stdout_pipe);
  pipe(stderr_pipe);
}

/**
 * @brief Destructor de subprocess
 */
subprocess::~subprocess() {
  close(stdin_pipe[0]);
  close(stdin_pipe[1]);
  close(stdout_pipe[0]);
  close(stdout_pipe[1]);
  close(stderr_pipe[0]);
  close(stderr_pipe[1]);
}

/**
 * @brief Método que verifica si el proceso hijo está en ejecución
 * @return Devuelve true si el proceso hijo está vivo (en ejecución) y false en caso contrario
 */
bool subprocess::is_alive() {
  return (child_pid != -1) && (waitpid(child_pid, nullptr, WNOHANG) == 0);
}

/**
 * @brief Método que crea un nuevo proceso hijo, y ejecuta el comando especificado por argumentos
 * @return Devuelve true si el proceso hijo está vivo (en ejecución) y false en caso contrario
 */
std::error_code subprocess::exec() {
  child_pid = fork();

  if (child_pid == -1) {
    return std::error_code(errno, std::system_category());
  }

  if (child_pid == 0) {  // Child process
    setup_child_process();
    
    std::vector<const char*> c_args;
    for (const auto& arg : args) { c_args.push_back(arg.c_str()); }
    c_args.push_back(nullptr); // execvp requires a null-terminated array

    execvp(args[0].c_str(), const_cast<char* const*>(c_args.data()));
    return std::error_code(errno, std::system_category());  // In case execvp fails
  }

  return std::error_code(0, std::system_category());
}

/**
 * @brief Método que espera a que el proceso hijo termine
 * @return Devuelve código de error 0 si la espera fue exitosa; de lo contrario, devuelve el código de error producido
 */
std::error_code subprocess::wait() {
  if (child_pid != -1) {
    int status;
    if (waitpid(child_pid, &status, 0) == -1) {
      return std::error_code(errno, std::system_category());
    }
    child_pid = -1;
  }
  return std::error_code(0, std::system_category());
}

/**
 * @brief Método que envía la señal SIGKILL al proceso hijo para forzar su terminación
 * @return Devuelve código de error 0 si la señal de finalización fue enviada con éxito; de lo contrario, devuelve el código de error producido
 */
std::error_code subprocess::kill() {
  if (child_pid != -1) {
    if (killpg(child_pid, SIGKILL) == -1) {
      return std::error_code(errno, std::system_category());
    }
    child_pid = -1;
  }
  return std::error_code(0, std::system_category());
}

/**
 * @brief Método para saber el PID del proceso hijo
 * @return Devuelve el PID del proceso hijo
 */
pid_t subprocess::pid() {
  return child_pid;
}

/**
 * @brief Métodos que proporcionan los descriptores de archivos de los pipes asociados.
 * @return Devuelven los descriptores de archivos asociados con la entrada/salida estándar del proceso hijo, dependiendo del tipo de redirección.
 */
int subprocess::stdin_fd() {
  return stdin_pipe[redirected_io == stdio::in ? 1 : 0];
}

int subprocess::stdout_fd() {
  return stdout_pipe[redirected_io == stdio::out ? 0 : (redirected_io == stdio::outerr ? 0 : 1)];
}

int subprocess::stderr_fd() {
  return stderr_pipe[redirected_io == stdio::err ? 0 : (redirected_io == stdio::outerr ? 1 : 2)];
}

/**
 * @brief Métodos que configura el entorno del proceso hijo cerrando extremos no utilizados de los pipes y duplicando los descriptores de archivos necesarios para redirigir la entrada/salida estándar.
 * @return Devuelven los descriptores de archivos asociados con la entrada/salida estándar del proceso hijo, dependiendo del tipo de redirección.
 */
void subprocess::setup_child_process() {
  close(stdin_pipe[1]);
  close(stdout_pipe[0]);
  close(stderr_pipe[0]);

  dup2(stdin_pipe[0], STDIN_FILENO);
  dup2(stdout_pipe[1], STDOUT_FILENO);
  dup2(stderr_pipe[1], STDERR_FILENO);

  close(stdin_pipe[0]);
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);
}