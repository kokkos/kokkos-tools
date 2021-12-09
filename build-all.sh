if [ "$#" -lt 1 ]; then
  ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
else
  ROOT_DIR=$1
fi
make -f $ROOT_DIR/common/kernel-filter/Makefile
make -f $ROOT_DIR/debugging/kernel-logger/Makefile
make -f $ROOT_DIR/profiling/memory-events/Makefile
make -f $ROOT_DIR/profiling/memory-hwm/Makefile
make -f $ROOT_DIR/profiling/memory-hwm-mpi/Makefile
make -f $ROOT_DIR/profiling/memory-usage/Makefile
make -f $ROOT_DIR/profiling/nvprof-connector/Makefile
make -f $ROOT_DIR/profiling/nvprof-focused-connector/Makefile
make -f $ROOT_DIR/profiling/papi-connector/Makefile
make -f $ROOT_DIR/profiling/simple-kernel-timer-json/Makefile
make -f $ROOT_DIR/profiling/simple-kernel-timer/Makefile
make -f $ROOT_DIR/profiling/space-time-stack/Makefile
make -f $ROOT_DIR/profiling/systemtap-connector/Makefile
if [ -z "${VTUNE_HOME}" ]; then
  echo ""
  echo "========================================="
  echo "Set VTUNE_HOME to build vtune connectors."
  echo "========================================="
  echo ""
else
  make -f $ROOT_DIR/profiling/vtune-connector/Makefile
  make -f $ROOT_DIR/profiling/vtune-focused-connector/Makefile
fi
