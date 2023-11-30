/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Sistemas Operativos 2023-2024
 *
 * @author Raúl González Acosta (alu0101543529@ull.edu.es)
 * @date   30/10/2023
 * @brief  netcp - Un programa en C++ que envía el contenido de archivos por la red mediante el uso de un socket configurado en una dirección IP y puerto UDP específico.
 */

#include "header_file/netcp.h"

int main(int argc, char *argv[]) {
  // Comprobamos si se especifican los argumentos necesarios para el correcto funcionamiento del programa
  if (argc <= 1) { 
    std::cerr << "Error: Faltan argumentos, use la opción -h para ver una descripción de su funcionamiento." << std::endl; 
    return 1;
  }

  std::vector<std::string_view> args(argv + 1, argv + argc);
  std::string output_filename;

  // Analizamos la línea de comandos
  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    // Opción -h | --help: Muestra ayuda sobre el funcionamiento del programa
    if (*it == "-h" || *it == "--help") { 
      show_help();
      return 0;
    }

    // Opción -o | --output: Para especificar un fichero que se leera y se enviara su contenido por la red
    if (*it == "-o" || *it == "--output") {
      if (++it != end) {
        output_filename = *it;
        std::cout << "El archivo escogido es: " << output_filename << std::endl;
        netcp_send_file(output_filename);
      }
      // Si no se ha especificado un archivo despues de la opción -o, mostraremos un mensaje de error y saldremos con código de error != 0
      else { 
        std::cerr << "Error: Falta un fichero, por favor introduzca la opción -h para ver una descripción de su funcionamiento." << std::endl; 
        return 1;
      }
    }

    //Opción -l: Se pone en modo "escucha" para recibir datos de un fichero
    if (*it == "-l") {
      if (++it != end) {
        output_filename = *it;
        std::cout << "El archivo escogido es: " << output_filename << std::endl;
        netcp_receive_file(output_filename);
      }
      // Si no se ha especificado un archivo despues de la opción -o, mostraremos un mensaje de error y saldremos con código de error != 0
      else { 
        std::cerr << "Error: Falta un fichero, por favor introduzca la opción -h para ver una descripción de su funcionamiento." << std::endl; 
        return 1;
      }
    }
  }

  return 0;
}