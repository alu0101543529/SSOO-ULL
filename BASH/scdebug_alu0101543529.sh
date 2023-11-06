#!/bin/bash

# Universidad de La Laguna
# Escuela Superior de Ingeniería y Tecnología
# Grado en Ingeniería Informática
# Sistemas Operativos 2023-2024

# Autor: Raúl González Acosta
# Fecha: 09/10/2023
# Correo Institucional: alu0101543529@ull.edu.es
# scdebug - Un script bash que mediante el comando "strace" ejecuta y monitoriza programas, y que con sus distintas opciones le da al usuario alternativas para ejecutarlos de la manera que crea conveniente.

# ----FLAGS----
  HELP_FLAG=false
  NATTCH_DO=false
  STO_DO=false
  DO_VERBOSE=false
  KILL_PROCESSES=false
  PATTCH_DO=false
  DO_COMMAND=false
  STOP_FLAG=false
  G_FLAG=false

# ----------------------------FUNCIONES------------------------------

  # Función que muestra el uso correcto del programa
  usage() {
    # Si el usuario ha seleccionado la opción -h, haciendo uso de la opción -S mostraremos otra función de ayuda especifica para la opción
    if [ "$STOP_FLAG" == true ]; then
      usage_stop
      exit 0
    else
      echo "Uso: ./scdebug [-h | --help] [-sto 'arg'] [-v | -vall] [-k] [prog [arg ...]] ] [-nattch progtoattach ...] [-pattch pid1 ... ]"
      echo "-h | --help: Muestra ayuda para el funcionamiento del comando"
      echo "-sto arg: Especifica opciones para strace, de modo que si se quieren pasar varias opciones, habrá que ponerlas todas entre comillas simples ('...')"
      echo "-v | -vall: Muestra, consultando los archivos de depuración generados por ejecuciones de strace, los resultados por pantalla. La opción -v el último archivo generado y -vall todos los archivos."
      echo "-k: Termina todos los procesos trazadores del usuario, así como todos los procesos trazados."
      echo "-nattch [progtoattach]: Monitoriza un programa indicado por el usuario, (cogiendo el pid del proceso mas reciente) en modo "attach"."
      echo "-pattch [progtoattach]: Monitoriza un proceso indicado por el usuario (pasando el pid como argumento) en modo "attach"."
      echo "prog: Nombre del programa a ejecutar y monitorizar"
    fi
  }

  # Función que muestra el uso correcto del programa (cuando se está ejecutando la opción -S)
  usage_stop() {
    echo "Uso: ./scdebug [-h] [-k] -S commName prog [arg...]"
    echo "-h | --help: Muestra ayuda para el funcionamiento del comando"
    echo "-k: Mantiene el script en segundo plano."
    echo "-S: Opción que detiene el proceso que indiquemos ("prog")."
    echo "commName: Nombre del comando a forzar su detención."
    echo "prog [arg...]: Programa ("prog") a ejecutar con sus argumentos ("arg")."
  }

  # Función para encontrar el PID del proceso más reciente con el nombre dado (para la opción -nattch)
  find_recent_process() {
    # En el parámetro posicional "1" encontraremos el programa en ejecución del cual queremos hallar su pid
    prog_name="$1"
    
    # Hallamos el pid (más reciente) con el comando "pgrep" (y la opción -n "newest") pasandole como parámetro el programa del parámetro posicional "1"
    pid=$(pgrep -n -u $USER $prog_name)
    echo $pid   # Imprimimos su pid para que el programa lo use cuando lo considere
  }

  # Función para ejecutar strace en segundo plano
  run_strace() {
    command="$1"                    # El pid del comando a ejecutar lo encontramos en el parámetro posicional "1"
    output_file="$2"                # El fichero de salida (con formato ".scdebug/PROG/trace_UUID.txt") lo encontramos en el parámetro posicional "2"
    if [ "$STO_DO" == true ]; then  # Si la opción -sto ha sido seleccionada, recibiremos las opciones que strace debiera ejecutar por el parámetro posicional "3"
      if [ -n "$STO_ARG" ]; then
        strace_options="$3"
      # Enviamos un mensaje de error si no se ha especificado ningún argumento para la opción -sto
      else
        echo "Error: Debe especificar una opción o opciones (entre comillas simples ('') para ejecutar la opción -sto." 1>&2
        exit 1
      fi
    else
      strace_options=""
    fi

    # Ejecutamos el comando "strace" pasándole como argumentos: 
    #  1) Las opciones a usar (especificadas en la opción -sto), si las hubiese (si no está vacio)
    #  2) El pid del comando a ejecutar (con la opción -p), o el comando (evaluado con el if)
    #  3) El fichero en el que guardaremos la salida de la monitorización del comando (con la opción -o)
    # Pero, si el comando strace falla (verificando si el código de salida del comando strace no es igual a 0) mostramos un mensaje de error

    if [ "$NATTCH_DO" == true ] || [ "$PATTCH_DO" == true ]; then   # Comprueba que tiene que hacer el attach con el pid, porque -nattch o -pattch está activo
      strace $strace_options -p $command -o $output_file 2>/dev/null || echo "Error: El comando strace ha fallado, con código de error "$?"." | tee -a "$HOME/.scdebug/errores.log" 1>&2
    else                                                            # Si no hemos activado -nattch o -pattch, haremos el strace con el comando que el usuario indique
      strace $strace_options -o $output_file $command 2>/dev/null || echo "Error: El comando strace ha fallado, con código de error "$?"." | tee -a "$HOME/.scdebug/errores.log" 1>&2
    fi
  }

  # Función para mostrar una traza individual (para la opción -v | -vall)
  show_trace() {
    trace_file="$1"
    echo "=============== COMMAND: $VERBOSE_PROG ======================="
    echo "=============== TRACE FILE: $trace_file ================="
    echo "=============== TIME: $(date -r "$trace_file") ================"
    # Mostramos las trazas del archivo que se recibe por el parametro posional "1"
    cat "$trace_file"
  }

  # Función para mostrar información de los procesos del usuario en monitorización
  show_traced_processes() {
    echo "=================== PROCESOS EN MONITORIZACIÓN ==================="
    printf "%-16s %-18s %-20s %-30s %-40s\n" "PID Tracee" "Tracee" "PID Tracer" "Tracer"

    # Para cada pid de procesos de usuario, comprobaremos si está siendo trazado
    for pid in $(ps -u $USER --no-headers -opid); do
      if [ -e "/proc/$pid/status" ]; then
        # En el parametro "tracer_pid" guardamos el pid del proceso trazador (si no está siendo trazado es 0)
        tracer_pid=$(cat /proc/$pid/status | grep "^TracerPid:" | tr -s "\t" " " | cut -d" " -f2)
        # En el parametro "tracee_name" guardamos el nombre del proceso trazado
        tracee_name=$(ps -o comm= -p $pid)
        if [ "$tracer_pid" -ne 0 ]; then
          # Si el proceso está siendo trazado, hallamos el nombre del proceso trazador
          tracer_name=$(ps -o comm= -p $tracer_pid)
          printf "%-16s %-18s %-20s %-30s %-40s\n" "$pid" "$tracee_name" "$tracer_pid" "$tracer_name"
        else
          printf "%-16s %-18s %-20s %-30s %-40s\n" "$pid" "$tracee_name" "------" "------"
        fi
      fi
    done
    echo "==================================================================="
  }
  
  # Función para comprobar los parámetros (para los acumuladores de argumentos)
  paramatres_check() {
    case "$1" in
      # Si en el parametro posicional "1" econtramos alguna de las opciones válidas, imprimimos un 1
      -sto | -nattch | -pattch | -k | -v | -vall | -h | --help )
        echo 1
      ;;
      # En otro caso mostramos un 0
      *)
        echo 0
      ;;
    esac
  }

  # Función para tirar procesos detenidos (para la opción -S)
  accion_stop() {
    command_name="$1"
    command="$2"

    # Forzamos el nombre de comando.
    echo -n "traced_$command_name" > /proc/$$/comm
    # Detenemos el script
    kill -SIGSTOP $$
    # Reanudamos después de la detención la ajecución del programa (con el comando "exec")
    exec "$command"
  }

# --------------------------------------------------------------------PROGRAMA PRINCIPAL--------------------------------------------------------------------

  # Analizamos los argumentos de la línea de comandos
  while [[ "$#" -gt 0 ]]; do
    # El argumento a analizar siempre estará en el parámetro posicional "1" (puesto que estaremos haciendo shift cada vez que leamos una funcionalidad)
    args="$1"
    
    case $args in
      # Opción [-h | --help] que muestra el uso correcto del programa
      -h | --help ) 
      HELP_FLAG=true      # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -h
      ;;
      
      # Opción [-sto arg], en el que el comando "strace" se ejecutará tomando como opciones lo indicado en el argumento "arg"
      -sto ) 
      STO_ARG="$2"        # El argumento/argumentos "arg" de las opciones para -sto lo encontramos en el parámetro posicional "2"
      STO_DO=true         # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -sto
      shift
      ;;
      
      # Opción [-nattch], en la que se monitorizan procesos que ya están en ejecución (modo attach)
      -nattch )
      NATTCH_DO=true      # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -nattch

      # Si no se ha especificado ningún programa, sacamos un mensaje de error.
      if [ -z "$2" ]; then
        echo "Error: No se ha especificado ningún programa/programas para poder adjuntar con la opción -nattch." 1>&2
        exit 1    # Salimos con código de error != 0, puesto que no se ha podido ejecutar la opción
      fi

      # Hacemos un bucle insertando en el parametro $NATTCH_PROG todos los programas, hasta que detecte otra opción válida.
      while [[ ! -z "$2" && $(paramatres_check "$2") == 0 ]]; do
        # El programa/programas en ejecución en modo attach lo encontramos en el parámetro posicional "2"
        NATTCH_PROG+=("$2")
        shift
      done
      ;;

      # Opción [-pattch], en la que se monitorizan procesos que ya están en ejecución por el pid (modo attach)
      -pattch )
      PATTCH_DO=true    # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -pattch
      
      # Si no se ha especificado ningún pid, sacamos un mensaje de error.
      if [ -z "$2" ]; then
        echo "Error: No se ha especificado ningún pid/pid's para poder adjuntar con la opción -pattch." 1>&2
        exit 1    # Salimos con código de error != 0, puesto que no se ha podido ejecutar la opción
      fi

      # Hacemos un bucle insertando en el parametro $PATTCH_PROG todos los pid's, hasta que detecte otra opción válida.
      while [[ ! -z "$2" && $(paramatres_check "$2") == 0 ]]; do
        # El pid del programa/programas en ejecución en modo attach lo encontramos en el parámetro posicional "2"
        PATTCH_PROG+=("$2")
        shift
      done
      ;;

      # Opción [-v | -vall], en la que se consultan los archivos de depuración generados por ejecuciones de strace.
      -v | -vall)
      DO_VERBOSE=true   # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -v

      # Si no se ha especificado ningún programa, sacamos un mensaje de error.
      if [ -z "$2" ]; then
        echo "Error: No se ha especificado ningún programa/programas para mostrar las trazas generadas." 1>&2
        exit 1    # Salimos con código de error != 0, puesto que no se ha podido ejecutar la opción
      fi

      if [ "$1" == "-v" ]; then
        # En este modo consulta el contenido del archivo de depuración más reciente entre los guardados para el programa especificado
        MODE="single"
      elif [ "$1" == "-vall" ]; then
        # En este modo consulta todos los archivos de depuración ordenados de más reciente a más antiguo para el programa especificado
        MODE="all"
      fi

      # Hacemos un bucle insertando en el parametro $VERBOSE_PROG todos los programas, hasta que detecte otra opción válida.
      while [[ ! -z "$2" && $(paramatres_check "$2") == 0 ]]; do
        VERBOSE_PROG+=("$2")   # El programa/programas que debemos mostrar la traza lo encontramos en el parámetro posicional "2"
        shift
      done
      ;;

      # Opción [-k], en la que se terminan todos los procesos trazadores del usuario, así como todos los procesos trazados.
      -k )
      KILL_PROCESSES=true   # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -k
      ;;

      # Opción [-S], en la que ejecuta procesos detenidos
      -S )
      STOP_FLAG=true        # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -S
      commName="$2"         # El nombre del grupo de comandos lo recogemos en el parámetro posicional "2"
      STOP_PROG="$3"        # El comando/comandos lo recogemos en el parámetro posicional "3"
      shift 3
      ;;

      # Opción [-g | -ge | -gc], en la que se crean tablas de ordenamiento según las opciones indicadas por el usuario.
      -g )
      G_FLAG=true           # Hacemos un "flag" para dejar constancia que el usuario ha indicado hacer la opción -g | -ge | -gc
      ;;

      # Si no hay más opciones, el resto se considera el comando a ejecutar
      * ) 
      DO_COMMAND=true       # Hacemos un "flag" para dejar constancia que el usuario ha indicado un comando a trazar
      COMMAND+="$1 "
      while [[ ! -z "$2" && $(paramatres_check "$2") == 0 ]]; do
        COMMAND+="$2 "
        shift
      done
      ;;
    esac
    shift
  done

  if [ "$HELP_FLAG" == true ]; then
    usage   # Mostramos la función de ayuda
    exit 0  # Sale con código de error 0, es decir no hay error.
  fi

  # Si no se seleccionan la opción [-v | -vall], mostraremos una tabla informativa con los procesos trazados y trazadores al principio
  if [ "$DO_VERBOSE" == false ]; then
    show_traced_processes   # Mostramos la tabla informativa
  fi

  # Crearemos el directorio .scdebug si no existe
  if [[ ! -d "$HOME/.scdebug" ]]; then
    mkdir -p "$HOME/.scdebug"
  fi

  # Comprobamos que la opción [-v | -vall] ha sido seleccionada (mediante el "flag")
  if [ "$DO_VERBOSE" == true ]; then
    # Comprobamos que se ha especificado un programa/programas para mostrar sus trazas
    if [ -n "$VERBOSE_PROG" ]; then
      # Definimos el directorio donde se guardan las trazas
      DEBUG_DIR="$HOME/.scdebug"

      # Modo de consulta [-v] (una sola traza)
      if [ "$MODE" == "single" ]; then
        for VERBOSE_PROG in "${VERBOSE_PROG[@]}"; do
          # Listamos todas las trazas ordenadas con la más reciente primero (con la opción -t) y nos quedamos con la ultima (con el comando head)
          LATEST_TRACE=$(ls -t "$DEBUG_DIR/$(basename "$VERBOSE_PROG")"/trace_*.txt 2>/dev/null | head -n 1)

          # Si la traza del programa no se encuentra salta un mensaje de error
          if [ -z "$LATEST_TRACE" ]; then
            echo "No se encontraron trazas para $VERBOSE_PROG" 1>&2
            exit 1      # Salimos con código de error != 0, puesto que el no se ha encontrado ninguna traza
          fi

          # Si existen trazas se imprimen usando la función "show_trace" 
          show_trace "$LATEST_TRACE"
          echo
        done
      fi

      # Modo de consulta [-vall] (todas las trazas)
      if [ "$MODE" == "all" ]; then
        for VERBOSE_PROG in "${VERBOSE_PROG[@]}"; do
          # Listamos todas las trazas ordenadas con la más reciente primero (con la opción -t)
          ALL_TRACES=$(ls -t "$DEBUG_DIR/$(basename "$VERBOSE_PROG")"/trace_*.txt 2>/dev/null)

          # Si no se encuentra ninguna traza salta un mensaje de error
          if [ -z "$ALL_TRACES" ]; then
            echo "No se encontraron trazas para $VERBOSE_PROG" 1>&2
            exit 1      # Salimos con código de error != 0, puesto que no se ha encontrado ninguna traza
          fi

          # Recorremos todas las trazas, e invocando a la función "show_trace", vamos imprimiendo todas las trazas
          for trace_file in $ALL_TRACES; do
            show_trace "$trace_file"
            echo
          done
        done
      fi
    else
      echo Error: No se ha especificado ningún programa/programas para mostrar sus trazas 1>&2
      exit 1    # Salimos con código de error != 0, puesto que no se ha especificado ningún programa
    fi
  fi

  # Comprobamos que la opción -nattch ha sido seleccionada (mediante el "flag")
  if [ "$NATTCH_DO" == true ]; then
    # Debemos comprobar si se ha especificado un programa con la opción -nattch (con la opcion -n nos aseguramos que no sea nula)
    if [ -n "$NATTCH_PROG" ]; then
      for NATTCH_PROG in "${NATTCH_PROG[@]}"; do
        # El pid deseado será el del programa en moodo attach, que hallaremos mediante la función "find_recent_process"
        TARGET_PID=$(find_recent_process $NATTCH_PROG)
        # Si el pid del programa no se encuentra salta un mensaje de error
        if [ -z $TARGET_PID ]; then
          echo "Error: No se encontró ningún proceso con el nombre $NATTCH_PROG" 1>&2
          exit 1    # Salimos con código de error != 0, puesto que no se ha encontrado ningún proceso con dicho pid
        fi

        # El fichero donde guardemos la salida de strace se encuentra en la ruta ".scdebug/$(nombre del programa)/trace_$UUID.txt"
        OUTPUT_FILE="$HOME/.scdebug/$NATTCH_PROG/trace_$(uuidgen).txt"
        # Si el fichero de salida está vació, o no se creó sacamos un mensaje de error
        if [ -z "$OUTPUT_FILE" ]; then
          echo "Error: No se pudo generar un fichero de salida del trazado correctamente." 1>&2
          exit 1      # Salimos con código de error != 0, puesto que no se ha generado el fichero correctamente
        fi

        # Crea los directorios necesarios si no estuviesen creados para almacenar la salida de strace en los directorios correspondientes (según el programa que se haya ejecutado)
        mkdir -p "$(dirname "$OUTPUT_FILE")"

        if [ "$STO_DO" == true ]; then
          run_strace "$TARGET_PID" "$OUTPUT_FILE" "$STO_ARG" &
        else
          run_strace "$TARGET_PID" "$OUTPUT_FILE" &
        fi
      done
      NATTCH_DO=false     # Establecemos el "flag" nuevamente a falso una vez ha terminado de hacer el attach
    else
      echo "Error: Debe especificar un programa/programas para ejecutar la opción -nattch para adjuntarse a un proceso existente." 1>&2
      exit 1
    fi
  fi

  # Comprobamos que la opción -pattch ha sido seleccionada (mediante el "flag")
  if [ "$PATTCH_DO" == true ]; then
    # Debemos comprobar si se ha especificado un pid con la opción -pattch (con la opcion -n nos aseguramos que no sea nula)
    if [ -n "$PATTCH_PROG" ]; then
      for PATTCH_PROG in "${PATTCH_PROG[@]}"; do
        PATTCH_COMMAND=$(ps -p "$PATTCH_PROG" -o comm=)
        # Si el pid del programa no existe salta un mensaje de error
        if [ -z "$PATTCH_COMMAND" ]; then
          echo "Error: No se encontró ningún proceso con el PID $PATTCH_PROG." 1>&2
          exit 1    # Salimos con código de error != 0, puesto que no se ha encontrado ningún proceso con dicho pid
        fi

        # El fichero donde guardemos la salida de strace se encuentra en la ruta ".scdebug/$(nombre del programa)/trace_$UUID.txt"
        OUTPUT_FILE="$HOME/.scdebug/$PATTCH_PROG/trace_$(uuidgen).txt"
        # Si el fichero de salida está vació, o no se creó sacamos un mensaje de error
        if [ -z "$OUTPUT_FILE" ]; then
          echo "Error: No se pudo generar un fichero de salida del trazado correctamente." 1>&2
          exit 1      # Salimos con código de error != 0, puesto que no se ha generado el fichero correctamente
        fi

        # Crea los directorios necesarios si no estuviesen creados para almacenar la salida de strace en los directorios correspondientes (según el pid del programa que se haya ejecutado)
        mkdir -p "$(dirname "$OUTPUT_FILE")"
        
        if [ "$STO_DO" == true ]; then
          run_strace "$PATTCH_PROG" "$OUTPUT_FILE" "$STO_ARG" &
        else
          run_strace "$PATTCH_PROG" "$OUTPUT_FILE" &
        fi
      done
      PATTCH_DO=false     # Establecemos el "flag" nuevamente a falso una vez ha terminado de hacer el attach
    else
      echo "Error: Debe especificar un pid/pid's para ejecutar la opción -pattch para adjuntarse a un proceso existente." 1>&2
      exit 1
    fi
  fi

  # Comprobamos que se haya especificado algún comando (con la opción -n comprobamos que no es nula)
  if [ -n "$COMMAND" ]; then
    for COMMAND in "${COMMAND[@]}"; do
      # El fichero donde guardemos la salida de strace se encuentra en la ruta ".scdebug/$(nombre del programa)/trace_$UUID.txt"
      OUTPUT_FILE="$HOME/.scdebug/$(basename $COMMAND)/trace_$(uuidgen).txt"
      # Si el fichero de salida está vació, o no se creó sacamos un mensaje de error
      if [ -z "$OUTPUT_FILE" ]; then
        echo "Error: No se pudo generar un fichero de salida del trazado correctamente." 1>&2
        exit 1      # Salimos con código de error != 0, puesto que no se ha generado el fichero correctamente
      fi

      mkdir -p "$(dirname "$OUTPUT_FILE")"

      if [ "$STO_DO" == true ]; then
        run_strace "$COMMAND" "$OUTPUT_FILE" "$STO_ARG" &
      else
        run_strace "$COMMAND" "$OUTPUT_FILE" &
      fi
    done
  fi

  # Modo para terminar procesos trazadores y trazados
  if [ "$KILL_PROCESSES" == "true" ]; then
    # Terminamos todos los procesos trazadores y trazados
    echo "Terminando todos los procesos trazadores y trazados del usuario..."

    for pid in $(ps -u $USER --no-header | tr -s " " "%" | cut -d"%" -f2); do
      if [ -e "/proc/$pid/status" ]; then
        TRACERS="$(cat /proc/$pid/status 2>/dev/null | grep "^TracerPid:" | tr -s "\t" " " | cut -d" " -f2) "
      fi

      if [[ -n "$TRACERS" && "$TRACERS" -ne 0 ]]; then
        kill -9 "$TRACERS" "$pid" 2>/dev/null
      fi
    done 

    if [ -z "$TRACERS" ]; then
      echo "No hay procesos trazadores en ejecución." 1>&2
      exit 1
    fi

    if [ -z "$pid" ]; then
      echo "No hay procesos trazados en ejecución." 1>&2
      exit 1
    fi

    echo "Todos los procesos trazadores y trazados han sido terminados."
    exit 0
  fi

  # Modo para tirar procesos detenidos, y trazarlos posteriormente
  if [ $STOP_FLAG == true ]; then
    # Si no se especifica el nombre del grupo de comandos, sacamos un mensaje de error
    if [ -z "$commName" ]; then
      echo "Error: Falta el nombre del comando." 1>&2
      exit 1
    fi

    # Si no se especifican los comandos a ejecutar, sacamos un mensaje de error
    if [ -z "$STOP_PROG" ]; then
      echo "Error: Falta el nombre del programa a ejecutar." 1>&2
      exit 1
    fi

    accion_stop "$commName" "$STOP_PROG"
    exit 0
  fi

  # Modo para generar tablas de procesos detenidos.
  if [ $G_FLAG == true ]; then
    # Guardamos en una variable todos los pids de los procesos que estan detenidos
    stopped_pids="$(ps x -o pid,stat,comm --no-headers | tr -s " " | grep "T traced_" | cut -d" " -f2)"
    # Si no existen procesos detenidos, no podremos generar la tabla, por tanto mostramos un mensaje de error
    if [ -z "$stopped_pids" ]; then
      echo "Error: No hay procesos detenidos en este momento, para poder generar las tablas de trazado." 1>&2
      exit 1        # Salimos con código de error != 0, porque no hemos podido realizar las tablas
    fi

    # Recorremos cada pid de proceso y lo trazamos con las opciones correspondientes dependiendo de las opciones indicadas
    for pid in $stopped_pids; do
      strace -c -U -p $pid 2>&1 >/dev/null
      sleep 0.1
      kill -SIGCONT $pid
    done
  fi

  exit 0