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

/**
 * @brief Función para mostrar ayuda sobre el funcionamiento del programa.
 */
void show_help() {
  std::cout << "Modo de uso: ./netcp [-h | --help ] [ -o | --output NombreArchivo ]" << std::endl << std::endl;
  std::cout << "Compilar con: g++ netcp.cc -o netcp" << std::endl;
  std::cout << "-h | --help: Muestra el funcionamiento del programa." << std::endl;
  std::cout << "-o | --output NombreArchivo: Envía por medio de la red el archivo indicado, leyendo su contenido." << std::endl;
}

/**
 * @brief Función para leer un fichero, y guardar su contenido en un buffer.
 * @param[in] fd: descriptor de fichero del que vamos a extraer su contenido.
 * @param[in] buffer: buffer en el que vamos a cargar el contenido del fichero.
 * @return Devuelve un código de error si no se ha podido leer correctamente del archivo, o un código de éxito si ocurre lo contrario.
 */
std::error_code read_file(int fd, std::vector<uint8_t>& buffer) {
  ssize_t bytes_read = read(fd, buffer.data(), buffer.size());

  // Si no se ha podido leer nada del fichero, mostraremos un mensaje de error y salimos con código de error != 0
  if (bytes_read < 0 || static_cast<size_t>(bytes_read) > buffer.size()) { 
    std::cerr << "Error: No se ha podido leer el archivo correctamente." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  // Si hemos podido leer el fichero, reajustaremos el tamaño del buffer en función de los bytes leídos
  buffer.resize(bytes_read);

  return std::error_code(0, std::system_category());
}

/**
 * @brief Función que crea un descriptor de fichero del socket en la dirección IP que le indiquemos.
 * @param[in] address: dirección IP a la cual enlazaremos el socket que creamos.
 * @return Devuelve el socket enlazado con la dirección y el puerto especificados por parámetros.
 */
make_socket_result make_socket(std::optional<sockaddr_in> address = std::nullopt) {
  // Creamos un socket de datagramas UDP (SOCK_DGRAM), en el dominio de direcciones IPv4 (AF_INET)
  int socket_fd_s = socket(AF_INET, SOCK_DGRAM, 0);
  
  // Si hay un error al crear el socket mostramos un mensaje de error, y salimos con código de error != 0
  if (socket_fd_s < 0) {
    std::cerr << "Error: No se ha podido crear el socket correctamente." << std::endl;
    std::error_code error(errno, std::system_category());
    return std::unexpected(error);
  }

  int result = bind(socket_fd_s, reinterpret_cast<const sockaddr*>(&address.value()), sizeof(address.value()));

  if (result < 0) {
    std::cerr << "Error: No se ha podido asignar una dirección IP correcta." << std::endl; 
    std::error_code error(errno, std::system_category());
    return std::unexpected(error);
  }

  return socket_fd_s;
}

/**
 * @brief Función que crea y configura una dirección IP especificada.
 * @param[in] ip_address: dirección IP que creamos y configuramos.
 * @param[in] port: numero de puerto en el cual enlazaremos la dirección IP creada.
 * @return Devuelve la direccioón IP configurada con el puerto especificado por parámetros.
 */
std::optional<sockaddr_in> make_ip_address(const std::optional<std::string> ip_address, uint16_t port = 0) {
  // Comprobaamos que se ha especificado una dirección IP, si no devolvemos un error
  if (!ip_address) { return std::nullopt; }
  
  // Configuramos la direeción IP en el puerto especificado por parámetros (port)
  sockaddr_in remote_address{};
  remote_address.sin_family = AF_INET;
  inet_aton(ip_address.value().c_str(), &remote_address.sin_addr);
  remote_address.sin_port = htons(port);

  //inet_pton(AF_INET, ip_address->c_str(), &remote_address.sin_addr);
  
  return remote_address;
}

/**
 * @brief Función que envía los datos de un fichero a través de un socket UDP a una dirección especificada por parámetros.
 * @param[in] socket_fd_s: descriptor de fichero del socket que hemos configurado con la dirección IP y puerto específico.
 * @param[in] buffer: buffer con el contenido de datos leídos del fichero.
 * @param[in] address: dirección IP a la cuál enviaremos el socket.
 * @return Devuelve un código de error si no se ha podido enviar un mensaje, o un código de éxito en caso contrario.
 */
std::error_code send_to(int socket_fd_s, const std::vector<uint8_t>& buffer, const sockaddr_in& address) {
  int bytes_sent = sendto(socket_fd_s, buffer.data(), buffer.size(), 0, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
  
  // Si no se ha podido enviar el contenido del fichero, mostramos un mensaje de error, y salimos con código de error != 0
  if (bytes_sent < 0) { 
    std::cerr << "Error: No se ha podido enviar el mensaje." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  return std::error_code(0, std::system_category());
}

/**
 * @brief Función que envía los datos de un fichero a través de un socket UDP a una dirección especificada por parámetros.
 * @param[in] fd_s: descriptor de fichero que hemos configurado con la dirección IP y puerto específico.
 * @param[in] buffer: buffer con el contenido de datos leídos del fichero.
 * @param[in] address: dirección IP a la cuál enviaremos el socket.
 * @return Devuelve un código de error si no se ha podido enviar un mensaje, o un código de éxito en caso contrario.
 */
std::error_code receive_from(int fd_s, std::vector<uint8_t>& buffer, sockaddr_in& address) {
  socklen_t address_length = sizeof(address);

  ssize_t bytes_received = recvfrom(fd_s, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&address), &address_length);

  if (bytes_received == -1) {
    // Si hay un error al recibir los datos en el socket mostramos un mensaje de error, y salimos con código de error != 0
    std::cerr << "Error: No se ha podido recibir datos por el socket." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  buffer.resize(bytes_received);

  return std::error_code(0, std::system_category());
}

std::error_code write_file(int fd_s, const std::vector<uint8_t>& buffer) {
  ssize_t bytes_written = write(fd_s, buffer.data(), buffer.size());

  if (bytes_written == -1) {
    // Si hay un error al escribir los datos recibidos por el socket en el archivo mostramos un mensaje de error, y salimos con código de error != 0
    std::cerr << "Error: No se han podido escribir los datos recibidos en el archivo." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  return std::error_code(0, std::system_category());
}

//-------------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Función que envía los datos de un fichero (especificado por parámetros) a través de un socket UDP haciendo uso de una dirección IP.
 * @param[in] filename: fichero del que leeremos su contenido y lo enviaremos haciendo uso de un socket, que hemos configurado con la dirección IP y puerto específico..
 * @return Devuelve un código de error si no se ha podido enviar un mensaje, o un código de éxito en caso contrario.
 */
std::error_code netcp_send_file(const std::string& filename) {
  // Verificamos el tamaño del archivo que recebimos por parámetros
  struct stat file_stat;
  //if (stat(filename.c_str(), &file_stat) != 0 || file_stat.st_size > 1024) {
  if (stat(filename.c_str(), &file_stat) != 0) {
    std::cerr << "Error: No se puede abrir el fichero " << filename << ". Compruebe que existe el archivo, y que no tiene un tamaño superior a 1 KiB." << std::endl;
    return std::error_code(errno, std::system_category());
  }
  
  // Abrimos el archivo y lo guardamos en un descriptor de fichero
  int fd_s = open(filename.c_str(), O_RDONLY, 0);
  // Si no se ha podido abrir, mostramos un mensaje de error y salimos con código de error != 0
  if (fd_s == -1) {
    std::cerr << "Error: No se puede abrir el fichero " << filename << "." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  // Creamos un buffer del tamaño del archivo especificado
  std::vector<uint8_t> buffer(file_stat.st_size);

  // Si no hemos podido leer el archivo y traspasar sus datos al buffer, mostramos un mensaje de error y salimos con código de error != 0
  if (read_file(fd_s, buffer)) {
    std::cerr << "Error: No se puede leer el fichero " << filename << "." << std::endl;
    // Cerramos el descriptor de fichero
    close(fd_s);
    
    return std::error_code(errno, std::system_category());
  }
  
  // Creamos y configuramos la dirección IP y puerto especificado
  auto address = make_ip_address("127.0.0.1", 0);
  // Si no hemos podido crear correctamente la dirección IP, mostramos un mensaje de error y salimos con código de error != 0
  if (!address) {
    std::cerr << "Error: No se ha podido crear la dirección IP." << std::endl;
    // Cerramos el descriptor de fichero
    close(fd_s);
    
    return std::error_code(errno, std::system_category());
  }
  
  // Creamos el socket con la dirección IP y puerto especificado previamente 
  auto socket_result = make_socket(*address);
  // Si no hemos podido crear correctamente el socket, mostramos un mensaje de error y salimos con código de error != 0
  if (!socket_result) {
    std::cerr << "Error: No se ha podido crear el socket." << std::endl;
    // Cerramos el descriptor de fichero
    close(fd_s);
    
    return std::error_code(errno, std::system_category());
  }
  // Si se ha creado correctamente lo guardamos en una variable
  int socket_fd_s = *socket_result;

  const char* netcp_port = std::getenv("NETCP_PORT");
  const char* netcp_ip = std::getenv("NETCP_IP");
  auto address_send = make_ip_address(netcp_ip, std::stoi(netcp_port));

  // Enviamos el mensaje del fichero que hemos leído previamente, en caso de fallo, mostramos un mensaje de error y salimos con código de error != 0
  if (send_to(socket_fd_s, buffer, *address_send)) {
    std::cerr << "Error: No se ha podido enviar el mensaje por el socket." << std::endl;
    // Cerramos el descriptor de fichero del archivo que leímos, y del socket que creamos
    close(socket_fd_s);
    close(fd_s);

    return std::error_code(errno, std::system_category());
  }
  // Si el mensaje se ha podido enviar cerramos tanto el descriptor de fichero del archivo que leímos como del socket que creamos
  close(socket_fd_s);
  close(fd_s);

  return std::error_code(errno, std::system_category());
}

/**
 * @brief Función que recibe los datos de un fichero a través de un socket UDP a una dirección IP específica.
 * @param[in] filename: descriptor de fichero del socket que hemos configurado con la dirección IP y puerto específico.
 * @return Devuelve un código de error si no se ha podido enviar un mensaje, o un código de éxito en caso contrario.
 */
std::error_code netcp_receive_file(const std::string& filename) {
  // Obtenemos el puerto y la dirección IP desde las variables de entorno
  const char* netcp_port = std::getenv("NETCP_PORT");
  const char* netcp_ip = std::getenv("NETCP_IP");

  uint16_t port = (netcp_port != nullptr) ? std::stoi(netcp_port) : 8080;
  std::optional<std::string> ip_address = (netcp_ip != nullptr) ? std::make_optional(netcp_ip) : "127.0.0.1";

  // Crear la dirección IP
  auto address = make_ip_address(ip_address, port);
  if (!address) {
    std::cerr << "Error: No se ha podido crear la dirección IP." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  // Crear el socket
  auto socket_result = make_socket(*address);
  if (!socket_result) {
    std::cerr << "Error: No se ha podido crear el socket correctamente." << std::endl;
    return std::error_code(errno, std::system_category());
  }

  int socket_fd = *socket_result;

  // Abrir el archivo de destino en modo escritura
  int fd_s = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd_s == -1) {
    std::cerr << "Error: No se puede abrir el fichero " << filename << "." << std::endl;
    close(socket_fd);
    return std::error_code(errno, std::system_category());
  }

  // Recibir datos por el socket y escribirlos en el archivo
  // Creamos un buffer del tamaño del archivo especificado
  std::vector<uint8_t> buffer(16ul * 1024 * 1024);

  while (true) {
    auto result = receive_from(socket_fd, buffer, address.value());

    if (result) {
      // Manejar el error de recepción
      std::cerr << "Error: No se han podido recibir los datos por el socket correctamente." << std::endl;
      close(fd_s);
      close(socket_fd);
      return std::error_code(errno, std::system_category());
    }

    // Si el buffer de datos esta vacío, es porque ya hemos llegado al fin de la transmisión
    if (buffer.empty()) { break; }

    // Escribiremos los datos que hemos ido leyendo en el archivo especificado por parámetros
    if (write_file(fd_s, buffer)) {
      std::cerr << "netcp_receive_file: error al escribir en el archivo '" << filename << "'\n";
      close(fd_s);
      close(socket_fd);
      return std::error_code(errno, std::system_category());
    }
  }

  close(fd_s);
  close(socket_fd);

  return std::error_code(0, std::system_category());
}