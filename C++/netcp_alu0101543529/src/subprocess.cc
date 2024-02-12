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
#include <iostream>

/**
 * @brief Constructor de subprocess
 * @param[in] args: representa los argumentos del comando a ejecutar
 * @param redirected_io: miembro de la enumeración `stdio` que indica cómo se manejará la entrada/salida estándar del proceso hijo.
 */
subprocess::subprocess(const std::vector<std::string>& args, subprocess::stdio redirected_io) : args(args), redirected_io(redirected_io), child_pid(-1) {
  if (pipe(std_pipe) < 0) { 
    std::cerr << "Error: No se ha podido cerar la tubería correctamente." << std::endl;
    exit(EXIT_FAILURE); 
  }
}

/**
 * @brief Destructor de subprocess
 */
subprocess::~subprocess() {
  close(std_pipe[0]);
  close(std_pipe[1]);
  close(std_pipe[2]);
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

  // Si no se ha podido cerar el proceso hijo (la llamada al sistema fork() falló), mostraremos un mensaje de error y saldremos con código de error != 0
  if (child_pid == -1) {
    std::cerr << "Error: No se ha podido crear el proceso hijo." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  // Dentro de este if solo entra el proceso hijo
  if (child_pid == 0) {
    setup_child_process();
    
    // Como la llamada execvp() requiere que el vector de argumentos termine en un puntero nulo, se lo insertamos creando un nuevo vector
    std::vector<const char*> c_args;
    for (const auto& arg : args) { c_args.push_back(arg.c_str()); }
    c_args.push_back(nullptr);

    // Si no se ha podido ejecutar el comando (la llamada al sistema execvp() falló), mostraremos un mensaje de error y saldremos con código de error != 0
    if (!(execvp(args[0].c_str(), const_cast<char* const*>(c_args.data())))) {
      std::cerr << "Error: No se ha podido ejecutar el comando." << std::endl;
      return std::error_code(errno, std::system_category());
    }
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
    if (killpg(child_pid, SIGTERM) == -1) {
      // Si no se ha podido matar al comando (la llamada al sistema killpg() falló), mostraremos un mensaje de error y saldremos con código de error != 0
      std::cerr << "Error: No se ha podido matar al proceso hijo con la señal SIGTERM." << std::endl;
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
int subprocess::stdin_fd() { return std_pipe[redirected_io == stdio::in ? 1 : 0]; }

int subprocess::stdout_fd() { return std_pipe[redirected_io == stdio::out ? 0 : (redirected_io == stdio::outerr ? 0 : 1)]; }

int subprocess::stderr_fd() { return std_pipe[redirected_io == stdio::err ? 0 : (redirected_io == stdio::outerr ? 1 : 2)]; }

/**
 * @brief Métodos que configura el entorno del proceso hijo cerrando extremos no utilizados de los pipes y duplicando los descriptores de archivos necesarios para redirigir la entrada/salida estándar.
 * @return Devuelven los descriptores de archivos asociados con la entrada/salida estándar del proceso hijo, dependiendo del tipo de redirección.
 */
void subprocess::setup_child_process() {
  close(std_pipe[0]);

  if (redirected_io == stdio::in) { dup2(std_pipe[0], STDIN_FILENO); }
  else if (redirected_io == stdio::out) { dup2(std_pipe[1], STDOUT_FILENO); }
  else if (redirected_io == stdio::err) { dup2(std_pipe[1], STDERR_FILENO); }

  close(std_pipe[1]);
}