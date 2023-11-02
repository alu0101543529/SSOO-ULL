#!/bin/bash

# Universidad de La Laguna
# Escuela Superior de Ingeniería y Tecnología
# Grado en Ingeniería Informática
# Sistemas Operativos

# Autor: Raúl González Acosta
# Fecha: 02/10/2023
# Correo Institucional: alu0101543529@ull.edu.es
# sysinfo - Un script que informa del estado del sistema

# -------------------------------------------------------------------

##### Constantes
TITLE="Información del sistema para $HOSTNAME"
RIGHT_NOW=$(date +"%x %r%Z")
TIME_STAMP="Actualizada el $RIGHT_NOW por $USER"

##### Estilos
TEXT_BOLD=$(tput bold)
TEXT_GREEN=$(tput setaf 2)
TEXT_RESET=$(tput sgr0)

##### Funciones
system_info() {
  echo "${TEXT_ULINE}Versión del sistema:${TEXT_RESET}"
  uname -a
  echo
}

show_uptime() {
  echo "${TEXT_ULINE}Tiempo de encendido del sistema:$TEXT_RESET"
  uptime
  echo
}

drive_space() {
  echo "${TEXT_ULINE}Espacio ocupado en las particiones/discos duros del sistema:$TEXT_RESET"
  df -h
  echo
}

home_space() {
  echo "${TEXT_ULINE}Espacio ocupado por /home/ en el sistema:$TEXT_RESET"
  if [ "$USER" == root ]; then
    du -hs /home/* | sort -rh
  else
    du -hs /home/$USER
  fi
  echo
}

##### Programa principal
cat << __EOF__

$TEXT_BOLD$TITLE$TEXT_RESET

$TEXT_GREEN$TIME_STAMP$TEXT_RESET

__EOF__

system_info
show_uptime
drive_space
home_space