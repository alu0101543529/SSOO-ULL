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

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <optional>
#include <expected>
#include <arpa/inet.h>

// Función para mostrar ayuda sobre el funcionamiento del programa.
void show_help();

// Función para leer un fichero, y guardar su contenido en un buffer.
std::error_code read_file(int, std::vector<uint8_t>&);

// Función que crea un descriptor de archivo del socket en la dirección IP que le indiquemos.
using make_socket_result = std::expected<int, std::error_code>;
make_socket_result make_socket(std::optional<sockaddr_in>);

// Función que crea y configura un socket con la dirección IP eespecificada.
std::optional<sockaddr_in> make_ip_address(const std::optional<std::string>, uint16_t);

// Función que envía datos a través de un socket UDP a una dirección especificada por parámetros.
std::error_code send_to(int, const std::vector<uint8_t>&, const sockaddr_in&);

// Función que envía los datos de un fichero (especificado por parámetros) a través de un socket UDP haciendo uso de una dirección IP.
std::error_code netcp_send_file(const std::string&);


std::error_code netcp_receive_file(const std::string&);