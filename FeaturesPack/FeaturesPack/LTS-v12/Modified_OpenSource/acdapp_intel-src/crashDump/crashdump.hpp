/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2019 Intel Corporation.
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
#include "utils.hpp"

#ifndef SPX_BMC_ACD
#include <linux/peci-ioctl.h>
#include <peci.h>
#endif

#include <array>
#include <vector>

extern "C" {
#ifndef SPX_BMC_ACD
#include <cjson/cJSON.h>
#include "safe_str_lib.h"
#else
#include "cJSON.h"
#include "libpeci4.h"
#include "crashdumpRunControl.h"
#endif
}
#define CRASHDUMP_PRINT(level, fmt, ...) fprintf(fmt, __VA_ARGS__)
#define CRASHDUMP_VALUE_LEN 6
typedef enum
{
    EMERG,
    ALERT,
    CRIT,
    ERR,
    WARNING,
    NOTICE,
    INFO,
    DEBUG,
} severity;

namespace cpuid
{
typedef enum
{
    STARTUP = 1,
    EVENT = 0,
    INVALID = 2,
    OVERWRITTEN = 3,
} cpuidState;
}

namespace record_type
{
constexpr const int offset = 24;
constexpr const int coreCrashLog = 0x04;
constexpr const int uncoreStatusLog = 0x08;
constexpr const int torDump = 0x09;
constexpr const int metadata = 0x0b;
constexpr const int pmInfo = 0x0c;
constexpr const int addressMap = 0x0d;
constexpr const int bmcAutonomous = 0x23;
constexpr const int mcaLog = 0x3e;
} // namespace record_type

namespace crashdump
{
constexpr char const* dbgStatusItemName = "status";
constexpr const char* dbgFailedStatus = "N/A";

namespace cpu
{
enum Model
{
    skx,
    clx,
    cpx,
    icx,
    icx2,
    numberOfModels,
};
namespace stepping
{
constexpr const uint8_t skx = 0;
constexpr const uint8_t clx = 6;
constexpr const uint8_t cpx = 10;
constexpr const uint8_t icx = 0;
constexpr const uint8_t icx2 = 4;
} // namespace stepping
} // namespace cpu

typedef enum
{
    ON = 1,
    OFF = 0,
    UNKNOWN = -1,
} pwState;

typedef enum
{
    BIG_CORE,
    MCA,
    UNCORE,
    TOR,
    PM_INFO,
    ADDRESS_MAP,
    METADATA,
    NUMBER_OF_SECTIONS,
} Section;

typedef struct
{
    char* name;
    Section section;
    int record_type;
} CrashdumpSection;

const CrashdumpSection sectionNames[NUMBER_OF_SECTIONS] = {
    {"big_core", BIG_CORE, record_type::coreCrashLog},
    {"MCA", MCA, record_type::mcaLog},
    {"uncore", UNCORE, record_type::uncoreStatusLog},
    {"TOR", TOR, record_type::torDump},
    {"PM_info", PM_INFO, record_type::pmInfo},
    {"address_map", ADDRESS_MAP, record_type::addressMap},
    {"METADATA", METADATA, record_type::metadata}};
const int NUMBER_OF_CPU_MODELS = crashdump::cpu::Model::numberOfModels;

typedef struct _InputFileInfo
{
    bool unique;
    char* filenames[NUMBER_OF_CPU_MODELS];
    cJSON* buffers[NUMBER_OF_CPU_MODELS];
} InputFileInfo;

typedef struct _JSONInfo
{
    char* filenamePtr;
    cJSON* bufferPtr;
} JSONInfo;

typedef struct COREMaskRead
{
    uint8_t coreMaskCc;
    int coreMaskRet;
} COREMaskRead;

typedef struct CHACountRead
{
    uint8_t chaCountCc;
    int chaCountRet;
} CHACountRead;

typedef struct CPUIDRead
{
    uint8_t cpuidCc;
    int cpuidRet;
    CPUModel cpuModel;
    uint8_t stepping;
    bool cpuidValid;
    cpuid::cpuidState source;
} CPUIDRead;

struct CPUInfo
{
    int clientAddr;
    cpu::Model model;
    uint64_t coreMask;
    uint64_t crashedCoreMask;
    uint8_t sectionMask = 0xff;
    size_t chaCount;
    pwState initialPeciWake;
    JSONInfo inputFile;
    CPUIDRead cpuidRead;
    CHACountRead chaCountRead;
    COREMaskRead coreMaskRead;
};
} // namespace crashdump

namespace revision
{
constexpr const int offset = 0;
constexpr const int revision1 = 0x01;
static int revision_uncore = 0;
} // namespace revision

namespace product_type
{
constexpr const int offset = 12;
constexpr const int clxSP = 0x2C;
constexpr const int cpx = 0x34;
constexpr const int skxSP = 0x2A;
constexpr const int icxSP = 0x1A;
constexpr const int bmcAutonomous = 0x23;
} // namespace product_type

/* Removed in ACD-0.9
namespace record_type
{
constexpr const int offset = 24;
constexpr const int coreCrashLog = 0x04;
constexpr const int uncoreStatusLog = 0x08;
constexpr const int torDump = 0x09;
constexpr const int metadata = 0x0b;
constexpr const int pmInfo = 0x0c;
constexpr const int addressMap = 0x0d;
constexpr const int bmcAutonomous = 0x23;
constexpr const int mcaLog = 0x3e;
} // namespace record_type
*/
inline static void logCrashdumpVersion(cJSON* parent,
                                       crashdump::CPUInfo& cpuInfo,
                                       int recordType)
{
    struct VersionInfo
    {
        crashdump::cpu::Model cpuModel;
        int data;
    };

#ifndef SPX_BMC_ACD
    static constexpr const std::array product{
#else
    static constexpr const std::array<VersionInfo, 5> product { {
#endif
        VersionInfo{crashdump::cpu::clx, product_type::clxSP},
        VersionInfo{crashdump::cpu::cpx, product_type::cpx},
        VersionInfo{crashdump::cpu::skx, product_type::skxSP},
        VersionInfo{crashdump::cpu::icx, product_type::icxSP},
        VersionInfo{crashdump::cpu::icx2, product_type::icxSP},
#ifndef SPX_BMC_ACD
    };
#else
    } };
#endif

#ifndef SPX_BMC_ACD
    static constexpr const std::array revision{
#else
    static constexpr const std::array<VersionInfo, 5> revision { {
#endif
        VersionInfo{crashdump::cpu::clx, revision::revision1},
        VersionInfo{crashdump::cpu::cpx, revision::revision1},
        VersionInfo{crashdump::cpu::skx, revision::revision1},
        VersionInfo{crashdump::cpu::icx, revision::revision1},
        VersionInfo{crashdump::cpu::icx2, revision::revision1},
#ifndef SPX_BMC_ACD
    };
#else
    } };
#endif

    int productType = 0;
    for (const VersionInfo& cpuProduct : product)
    {
        if (cpuInfo.model == cpuProduct.cpuModel)
        {
            productType = cpuProduct.data;
        }
    }

    int revisionNum = 0;
    for (const VersionInfo& cpuRevision : revision)
    {
        if (cpuInfo.model == cpuRevision.cpuModel)
        {
            revisionNum = cpuRevision.data;
        }
    }

    if (recordType == record_type::uncoreStatusLog)
    {
        revisionNum = revision::revision_uncore;
    }

    // Build the version number:
    //  [31:30] Reserved
    //  [29:24] Crash Record Type
    //  [23:12] Product
    //  [11:8] Reserved
    //  [7:0] Revision
    int version = recordType << record_type::offset |
                  productType << product_type::offset |
                  revisionNum << revision::offset;
    char versionString[64];
    cd_snprintf_s(versionString, sizeof(versionString), "0x%x", version);
    cJSON_AddStringToObject(parent, "_version", versionString);
}
