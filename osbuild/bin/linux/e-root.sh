#!/bin/bash

if [[ -z "${E_ROOT}" ]]; then
  export E_ROOT="/coderoot"
fi

if [[ -z "${E_BUILD_BIN}" ]]; then
  export E_BUILD_BIN="${E_ROOT}/eosal/build/bin/linux"
  export PATH=${PATH}:${E_BUILD_BIN}
fi

