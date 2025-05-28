# Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
# SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

source /data/emsdk/emsdk_env.sh
bash build_wasm.sh \
          --simd-mode=both \
          --configure \

mv dist output