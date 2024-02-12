/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Sistemas Operativos 2023-2024
 *
 * @author Raúl González Acosta (alu0101543529@ull.edu.es)
 * @date   30/10/2023
 * @brief  Declaración de la clase Subprocess
*/

#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <vector>
#include <string>
#include <system_error>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

class subprocess {
 public:
  enum class stdio {
    in,
    out,
    err,
    outerr
  };

  // CONSTRUCTOR
  subprocess(const std::vector<std::string>& args, subprocess::stdio redirected_io);
  // DESTRUCTOR
  ~subprocess();

  // MÉTODO PARA SABER SI EL PROCESO HIJO ESTÁ EN EJECUCUÓN
  bool is_alive();

  // MÉTODO PARA CREAR UN NUEVO PROCESO HIJO
  std::error_code exec();

  // MÉTODO PARA ESPERAR A QUE EL PROCESO HIJO TERMINE
  std::error_code wait();

  // MÉTODO PARA ENVIAR LA SEÑAL SIGKILL AL PROCESO HIJO FORZANDO SU TERMINACIÓN
  std::error_code kill();

  // MÉTODO PARA SABER EL PID DEL PROCESO HIJO
  pid_t pid();

  // MÉTODOS PARA DEVOLVER LOS DESCRIPTORES DE FICHERO I/O DEL PROCESO HIJO, DEPENDIENDO DE TU REDIRECCIÓN
  int stdin_fd();
  int stdout_fd();
  int stderr_fd();

 private:
  // MÉTODO PARA CONFIGURAR EL ENTORNO DEL PROCESO HIJO
  void setup_child_process();

  // Atributos que guardan los argumentos del comando a ejecutar, así como un indicador de cómo se manejará la entrada/salida estándar del proceso hijo, y el PID del proceso hijo
  std::vector<std::string> args;
  subprocess::stdio redirected_io;
  pid_t child_pid;
  int std_pipe[2];
};

#endif // SUBPROCESS_H
