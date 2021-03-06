/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 *
 ******************************************************************************/

#pragma once

#include "crashdump.hpp"
#ifndef SPX_BMC_ACD
#include <cjson/cJSON.h>
#endif
#include <stdint.h>

typedef struct
{
    crashdump::cpu::Model cpuModel;
    int (*logOemDataVx)(crashdump::CPUInfo& cpuInfo, cJSON* pJsonChild);
} SOemDataVx;

int logOemData(crashdump::CPUInfo& cpuInfo, cJSON* pJsonChild);
