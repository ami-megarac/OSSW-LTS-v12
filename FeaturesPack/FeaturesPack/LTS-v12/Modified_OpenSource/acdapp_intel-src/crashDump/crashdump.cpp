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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#ifndef SPX_BMC_ACD
#include <peci.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef SPX_BMC_ACD
#include <systemd/sd-id128.h>
#endif
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef SPX_BMC_ACD
#include <filesystem>
#endif
#include <fstream>
#include <regex>
#ifndef SPX_BMC_ACD
#include <sdbusplus/asio/object_server.hpp>
#endif
#include <sstream>
#include <vector>

extern "C" {
#ifndef SPX_BMC_ACD
#include <cjson/cJSON.h>

#include "safe_str_lib.h"
#endif
}

#include "CrashdumpSections/AddressMap.hpp"
#include "CrashdumpSections/BigCore.hpp"
#include "CrashdumpSections/CoreMca.hpp"
#include "CrashdumpSections/MetaData.hpp"
#include "CrashdumpSections/PowerManagement.hpp"
#include "CrashdumpSections/SqDump.hpp"
#include "CrashdumpSections/TorDump.hpp"
#include "CrashdumpSections/Uncore.hpp"
#include "CrashdumpSections/UncoreMca.hpp"
#include "utils.hpp"
#ifdef CONFIG_SPX_FEATURE_OEMDATA_SECTION
#include "CrashdumpSections/OemData.hpp"
#endif

int max_cpus= MAX_CLIENT_ADDR - MIN_CLIENT_ADDR + 1;
cJSON* selectAndReadInputFile(crashdump::cpu::Model cpuModel, char** filename,
                              bool isTelemetry);

namespace crashdump
{
#ifndef SPX_BMC_ACD
static std::vector<crashdump::CPUInfo> cpuInfo;
static boost::asio::io_service io;
static std::shared_ptr<sdbusplus::asio::connection> conn;
static std::shared_ptr<sdbusplus::asio::object_server> server;
static std::shared_ptr<sdbusplus::asio::dbus_interface> logIface;
static std::vector<
    std::pair<std::string, std::shared_ptr<sdbusplus::asio::dbus_interface>>>
    storedLogIfaces;

constexpr char const* crashdumpService = "com.intel.crashdump";
constexpr char const* crashdumpPath = "/com/intel/crashdump";
#else
static std::vector<crashdump::CPUInfo> cpuInfo;
constexpr char const* crashdumpPath = "/var/crashdump/json";
#endif
constexpr char const* crashdumpInterface = "com.intel.crashdump";
constexpr char const* crashdumpOnDemandPath = "/com/intel/crashdump/OnDemand";
constexpr char const* crashdumpTelemetryPath = "/com/intel/crashdump/Telemetry";
constexpr char const* crashdumpStoredInterface = "com.intel.crashdump.Stored";
constexpr char const* crashdumpDeleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";
constexpr char const* crashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr char const* crashdumpTelemetryInterface =
    "com.intel.crashdump.Telemetry";
constexpr char const* crashdumpRawPeciInterface =
    "com.intel.crashdump.SendRawPeci";
#ifndef SPX_BMC_ACD
//static const std::filesystem::path crashdumpDir = "/tmp/crashdumps";
static const std::filesystem::path crashdumpDir = "/tmp/crashdump/output";
static const std::string crashdumpFileRoot{"crashdump_ondemand_"};
static const std::string crashdumpTelemetryFileRoot{"telemetry_"};
static const std::string crashdumpPrefix{"crashdump_"};
#else
static const char* crashdumpDir = "/var/crashdump/json";
#endif

constexpr char const* triggerTypeOnDemand = "On-Demand";
constexpr int const vcuPeciWake = 5;

static uint64_t tsToNanosecond(timespec* ts)
{
    return (ts->tv_sec * (uint64_t)1e9 + ts->tv_nsec);
}

static void logRunTime(cJSON* parent, timespec* start, char* key)
{
    char timeString[64];
    timespec finish = {};
    uint64_t timeVal = 0;

    clock_gettime(CLOCK_MONOTONIC, &finish);
    uint64_t runTimeInNs = tsToNanosecond(&finish) - tsToNanosecond(start);

    timeVal = runTimeInNs;

    // only log the last metaData run
    cJSON_GetObjectItemCaseSensitive(parent, "_time");

    cd_snprintf_s(timeString, sizeof(timeString), "%.2fs",
                  (double)timeVal / 1e9);
    cJSON_AddStringToObject(parent, key, timeString);

    clock_gettime(CLOCK_MONOTONIC, start);
}

static const std::string getUuid()
{
    std::string ret;
#ifndef SPX_BMC_ACD
    sd_id128_t appId = SD_ID128_MAKE(e0, e1, 73, 76, 64, 61, 47, da, a5, 0c, d0,
                                     cc, 64, 12, 45, 78);
    sd_id128_t machineId = SD_ID128_NULL;

    if (sd_id128_get_machine_app_specific(appId, &machineId) == 0)
    {
        std::array<char, SD_ID128_STRING_MAX> str;
        ret = sd_id128_to_string(machineId, str.data());
        ret.insert(8, 1, '-');
        ret.insert(13, 1, '-');
        ret.insert(18, 1, '-');
        ret.insert(23, 1, '-');
    }
#endif

    return ret;
}

static void getClientAddrs(std::vector<CPUInfo>& cpuInfo)
{
#ifndef SPX_BMC_ACD
    for (int cpu = 0, addr = MIN_CLIENT_ADDR; addr <= MAX_CLIENT_ADDR;
         cpu++, addr++)
#else
    for (int cpu = 0, addr = MIN_CLIENT_ADDR; cpu < max_cpus && addr <= MAX_CLIENT_ADDR;
         cpu++, addr++)
#endif
    {
        if (peci_Ping(addr) == PECI_CC_SUCCESS)
        {
            cpuInfo.emplace_back();
            cpuInfo[cpu].clientAddr = addr;
        }
    }
}

static bool savePeciWake(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;
    uint32_t peciRdValue = 0;
    for (CPUInfo& cpu : cpuInfo)
    {
        retval =
            peci_RdPkgConfig(cpu.clientAddr, vcuPeciWake, ON, sizeof(uint32_t),
                             (uint8_t*)&peciRdValue, &cc);
        if (retval != PECI_CC_SUCCESS)
        {
            CRASHDUMP_PRINT(
                ERR, stderr,
                "Cannot read Wake_on_PECI-> addr: (0x%x), ret: (0x%x), \
                        cc: (0x%x)\n",
                cpu.clientAddr, retval, cc);
            cpu.initialPeciWake = UNKNOWN;
            continue;
        }
        cpu.initialPeciWake = (peciRdValue & 0x1) ? ON : OFF;
    }
    return true;
}

static bool setPeciWake(std::vector<CPUInfo>& cpuInfo, pwState desiredState)
{
    uint8_t cc = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;
    int writeValue = OFF;
    for (CPUInfo& cpu : cpuInfo)
    {
        if ((cpu.initialPeciWake == ON) || (cpu.initialPeciWake == UNKNOWN))
            continue;
        writeValue = static_cast<int>(desiredState);
        retval = peci_WrPkgConfig(cpu.clientAddr, vcuPeciWake, writeValue,
                                  writeValue, sizeof(uint32_t), &cc);
        if (retval != PECI_CC_SUCCESS)
        {
            CRASHDUMP_PRINT(
                ERR, stderr,
                "Cannot set Wake_on_PECI-> addr: (0x%x), ret: (0x%x), cc: "
                "(0x%x)\n",
                cpu.clientAddr, retval, cc);
        }
    }
    return true;
}

static bool checkPeciWake(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;
    uint32_t peciRdValue = 0;
    for (CPUInfo& cpu : cpuInfo)
    {
        retval =
            peci_RdPkgConfig(cpu.clientAddr, vcuPeciWake, ON, sizeof(uint32_t),
                             (uint8_t*)&peciRdValue, &cc);
        if (retval != PECI_CC_SUCCESS)
        {
            CRASHDUMP_PRINT(
                ERR, stderr,
                "Cannot read Wake_on_PECI-> addr: (0x%x), ret: (0x%x), \
                        cc: (0x%x)\n",
                cpu.clientAddr, retval, cc);
            continue;
        }
        if (peciRdValue != 1)
        {
            CRASHDUMP_PRINT(ERR, stderr, "Wake_on_PECI in OFF state: (0x%x)\n",
                            cpu.clientAddr);
        }
    }
    return true;
}

static void loadInputFiles(std::vector<CPUInfo>& cpuInfo,
                           InputFileInfo* inputFileInfo, bool isTelemetry)
{
    int uniqueCount = 0;
    cJSON* defaultStateSection = NULL;
    bool enable = false;

    for (CPUInfo& cpu : cpuInfo)
    {
        // read and allocate memory for crashdump input file
        // if it hasn't been read before
        if (inputFileInfo->buffers[cpu.model] == NULL)
        {
            inputFileInfo->buffers[cpu.model] = selectAndReadInputFile(
                cpu.model, &inputFileInfo->filenames[cpu.model], isTelemetry);
            if (inputFileInfo->buffers[cpu.model] != NULL)
            {
                uniqueCount++;
            }
        }

        inputFileInfo->unique = (uniqueCount <= 1);
        cpu.inputFile.filenamePtr = inputFileInfo->filenames[cpu.model];
        cpu.inputFile.bufferPtr = inputFileInfo->buffers[cpu.model];

        // Get and Check global enable/disable value from "DefaultState"
        defaultStateSection = getCrashDataSection(
            inputFileInfo->buffers[cpu.model], "DefaultState", &enable);
        int defaultStateEnable = 1;
        if (defaultStateSection != NULL)
        {
            //strcmp_s(defaultStateSection->valuestring, CRASHDUMP_VALUE_LEN,
            //         "Enable", &defaultStateEnable);
            defaultStateEnable = strncmp(defaultStateSection->valuestring, "Enable", CRASHDUMP_VALUE_LEN);
        }
        else
        {
            CRASHDUMP_PRINT(ERR, stderr, "Missing \"DefaultState\" in %s\n",
                            inputFileInfo->filenames[cpu.model]);
        }
        if (defaultStateEnable == ENABLE)
        {
            // Check local enable "_record_enable"
            for (uint8_t i = 0; i < crashdump::NUMBER_OF_SECTIONS; i++)
            {
                getCrashDataSection(inputFileInfo->buffers[cpu.model],
                                    crashdump::sectionNames[i].name, &enable);
                enable ? SET_BIT(cpu.sectionMask, i)
                       : CLEAR_BIT(cpu.sectionMask, i);
            }
        }
        else
        {
            cpu.sectionMask = 0x00;
        }
    }
}

static bool getCPUModels(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    bool ret = false;
    EPECIStatus retval = PECI_CC_SUCCESS;

    for (CPUInfo& cpu : cpuInfo)
    {
        CPUModel cpuModel{};
        uint8_t stepping = 0;

        retval = peci_GetCPUID(cpu.clientAddr, &cpuModel, &stepping, &cc);
        if (retval != PECI_CC_SUCCESS)
        {
            fprintf(stderr, "Cannot get CPUID! ret: (0x%x), cc: (0x%x)\n",
                    retval, cc);
            continue;
        }

        // Check that it is a supported CPU
        switch (cpuModel)
        {
            case skx:
                if (stepping >= cpu::stepping::cpx)
                {
                    fprintf(stderr, "CPX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::cpx;
                }
                else if (stepping >= cpu::stepping::clx)
                {
                    fprintf(stderr, "CLX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::clx;
                }
                else
                {
                    fprintf(stderr, "SKX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::skx;
                }
                ret = true;
                break;
            case icx:
                fprintf(stderr, "ICX detected (CPUID 0x%x)\n",
                        cpuModel | stepping);
                if (stepping >= cpu::stepping::icx2)
                {
                    cpu.model = cpu::icx2;
                }
                else
                {
                    cpu.model = cpu::icx;
                }
                ret = true;
                break;
            default:
                fprintf(stderr, "Unsupported CPUID 0x%x\n",
                        cpuModel | stepping);
                break;
        }
    }
    return ret;
}

static bool maxCPUS(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    bool ret = false;
    EPECIStatus retval = PECI_CC_SUCCESS;
    int cpuCount= 0 ;

//fprintf(stderr, "\n**maxCPUs call**\n");
    for (CPUInfo& cpu : cpuInfo)
    {
        CPUModel cpuModel{};
        uint8_t stepping = 0;

        retval = peci_GetCPUID(cpu.clientAddr, &cpuModel, &stepping, &cc);
        if (retval != PECI_CC_SUCCESS)
        {
            fprintf(stderr, "Cannot get CPUID! ret: (0x%x), cc: (0x%x)\n",
                    retval, cc);
            continue;
        }

        // Check that it is a supported CPU
        switch (cpuModel)
        {
            case skx:
                if (stepping >= cpu::stepping::cpx)
                {
                    fprintf(stderr, "CPX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::cpx;
                }
                else if (stepping >= cpu::stepping::clx)
                {
                    fprintf(stderr, "CLX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::clx;
                }
                else
                {
                    fprintf(stderr, "SKX detected (CPUID 0x%x)\n",
                            cpuModel | stepping);
                    cpu.model = cpu::skx;
                }
                ret = true;
                break;
            case icx:
                fprintf(stderr, "ICX detected (CPUID 0x%x)\n",
                        cpuModel | stepping);
		cpuCount++;
                if (stepping >= cpu::stepping::icx2)
                {
                    cpu.model = cpu::icx2;
                }
                else
                {
                    cpu.model = cpu::icx;
                }
                ret = true;
                break;
            default:
                fprintf(stderr, "Unsupported CPUID 0x%x\n",
                        cpuModel | stepping);
                break;
        }
    }
  max_cpus=cpuCount;
  //fprintf(stderr, "\n**maxCPUs call: max cpu : %d**\n", max_cpus);
return ret;
}


static bool getCoreMasks(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;

    for (CPUInfo& cpu : cpuInfo)
    {
        switch (cpu.model)
        {
            case cpu::cpx:
            case cpu::clx:
            case cpu::skx:
                // RESOLVED_CORES Local PCI B1:D30:F3 Reg 0xB4
                uint32_t coreMask;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 1, 30, 3, 0xB4,
                                               sizeof(coreMask),
                                               (uint8_t*)&coreMask, &cc);
                cpu.coreMaskRead.coreMaskCc = cc;
                cpu.coreMaskRead.coreMaskRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find coreMask! ret: (0x%x), cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                cpu.coreMask = coreMask;
                break;
            case cpu::icx:
            case cpu::icx2:
                // RESOLVED_CORES Local PCI B14:D30:F3 Reg 0xD0 and 0xD4
                uint32_t coreMask0;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 14, 30, 3, 0xD0,
                                               sizeof(coreMask0),
                                               (uint8_t*)&coreMask0, &cc);
                cpu.coreMaskRead.coreMaskCc = cc;
                cpu.coreMaskRead.coreMaskRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find coreMask0! ret: (0x%x), cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                uint32_t coreMask1;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 14, 30, 3, 0xD4,
                                               sizeof(coreMask1),
                                               (uint8_t*)&coreMask1, &cc);
                cpu.coreMaskRead.coreMaskCc = cc;
                cpu.coreMaskRead.coreMaskRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find coreMask1! ret: (0x%x), cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                cpu.coreMask = coreMask1;
                cpu.coreMask <<= 32;
                cpu.coreMask |= coreMask0;
                break;
            default:
                return false;
        }
    }
    return true;
}

static bool getCHACounts(std::vector<CPUInfo>& cpuInfo)
{
    uint8_t cc = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;

    for (CPUInfo& cpu : cpuInfo)
    {
        switch (cpu.model)
        {
            case cpu::cpx:
            case cpu::clx:
            case cpu::skx:
                // LLC_SLICE_EN Local PCI B1:D30:F3 Reg 0x9C
                uint32_t chaMask;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 1, 30, 3, 0x9C,
                                               sizeof(chaMask),
                                               (uint8_t*)&chaMask, &cc);
                cpu.chaCountRead.chaCountCc = cc;
                cpu.chaCountRead.chaCountRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find chaMask! ret: (0x%x), cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                cpu.chaCount = __builtin_popcount(chaMask);
                break;
            case cpu::icx:
            case cpu::icx2:
                // LLC_SLICE_EN Local PCI B14:D30:F3 Reg 0x9C and 0xA0
                uint32_t chaMask0;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 14, 30, 3, 0x9C,
                                               sizeof(chaMask0),
                                               (uint8_t*)&chaMask0, &cc);
                cpu.chaCountRead.chaCountCc = cc;
                cpu.chaCountRead.chaCountRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find chaMask0! ret: (0x%x),  cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                uint32_t chaMask1;
                retval = peci_RdPCIConfigLocal(cpu.clientAddr, 14, 30, 3, 0xA0,
                                               sizeof(chaMask1),
                                               (uint8_t*)&chaMask1, &cc);
                cpu.chaCountRead.chaCountCc = cc;
                cpu.chaCountRead.chaCountRet = retval;
                if (retval != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(
                        ERR, stderr,
                        "Cannot find chaMask1! ret: (0x%x), cc: (0x%x)\n",
                        retval, cc);
                    return false;
                }
                cpu.chaCount =
                    __builtin_popcount(chaMask0) + __builtin_popcount(chaMask1);
                break;
            default:
                return false;
        }
    }
    return true;
}

static void getCPUID(CPUInfo& cpuInfo)
{
    uint8_t cc = 0;
    CPUModel cpuModel{};
    uint8_t stepping = 0;
    EPECIStatus retval = PECI_CC_SUCCESS;

    retval = peci_GetCPUID(cpuInfo.clientAddr, &cpuModel, &stepping, &cc);
    cpuInfo.cpuidRead.cpuModel = cpuModel;
    cpuInfo.cpuidRead.stepping = stepping;
    cpuInfo.cpuidRead.cpuidCc = cc;
    cpuInfo.cpuidRead.cpuidRet = retval;
    if (retval != PECI_CC_SUCCESS)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Cannot get CPUID! ret: (0x%x), cc: (0x%x)\n", retval,
                        cc);
    }
}

static void parseCPUInfo(CPUInfo& cpuInfo, cpuid::cpuidState cpuState)
{
    switch (cpuInfo.cpuidRead.cpuModel)
    {
        case skx:
            if (cpuInfo.cpuidRead.stepping >= cpu::stepping::cpx)
            {
                CRASHDUMP_PRINT(INFO, stderr, "CPX detected (CPUID 0x%x)\n",
                                cpuInfo.cpuidRead.cpuModel |
                                    cpuInfo.cpuidRead.stepping);
                cpuInfo.model = cpu::cpx;
            }
            else if (cpuInfo.cpuidRead.stepping >= cpu::stepping::clx)
            {
                CRASHDUMP_PRINT(INFO, stderr, "CLX detected (CPUID 0x%x)\n",
                                cpuInfo.cpuidRead.cpuModel |
                                    cpuInfo.cpuidRead.stepping);
                cpuInfo.model = cpu::clx;
            }
            else
            {
                CRASHDUMP_PRINT(INFO, stderr, "SKX detected (CPUID 0x%x)\n",
                                cpuInfo.cpuidRead.cpuModel |
                                    cpuInfo.cpuidRead.stepping);
                cpuInfo.model = cpu::skx;
            }
            cpuInfo.cpuidRead.cpuidValid = true;
            cpuInfo.cpuidRead.source = cpuState;
            break;
        case icx:
            CRASHDUMP_PRINT(INFO, stderr, "ICX detected (CPUID 0x%x)\n",
                            cpuInfo.cpuidRead.cpuModel |
                                cpuInfo.cpuidRead.stepping);
            if (cpuInfo.cpuidRead.stepping >= cpu::stepping::icx2)
            {
                cpuInfo.model = cpu::icx2;
            }
            else
            {
                cpuInfo.model = cpu::icx;
            }
            cpuInfo.cpuidRead.cpuidValid = true;
            cpuInfo.cpuidRead.source = cpuState;
            break;
        case icxd:
            CRASHDUMP_PRINT(INFO, stderr, "ICXD detected (CPUID 0x%x)\n",
                            cpuInfo.cpuidRead.cpuModel |
                                cpuInfo.cpuidRead.stepping);
            cpuInfo.model = cpu::icx2;
            cpuInfo.cpuidRead.cpuidValid = true;
            cpuInfo.cpuidRead.source = cpuState;
            break;
        default:
            CRASHDUMP_PRINT(ERR, stderr, "Unsupported CPUID 0x%x\n",
                            cpuInfo.cpuidRead.cpuModel |
                                cpuInfo.cpuidRead.stepping);
            cpuInfo.cpuidRead.cpuidValid = false;
            break;
    }
}

static void overwriteCPUInfo(std::vector<CPUInfo>& cpuInfo)
{
    bool found = false;
    cpu::Model defaultModel;
    CPUModel defaultCpuModel;
    uint8_t defaultStepping;
    for (CPUInfo& cpu : cpuInfo)
    {
        if (cpu.cpuidRead.cpuidValid)
        {
            defaultModel = cpu.model;
            defaultCpuModel = cpu.cpuidRead.cpuModel;
            defaultStepping = cpu.cpuidRead.stepping;
            found = true;
            break;
        }
    }
    for (CPUInfo& cpu : cpuInfo)
    {
        if (!cpu.cpuidRead.cpuidValid)
        {
            if (found)
            {
                cpu.model = defaultModel;
                cpu.cpuidRead.cpuModel = defaultCpuModel;
                cpu.cpuidRead.stepping = defaultStepping;
                cpu.cpuidRead.source = cpuid::OVERWRITTEN;
                cpu.cpuidRead.cpuidCc = PECI_DEV_CC_SUCCESS;
                cpu.cpuidRead.cpuidRet = PECI_CC_SUCCESS;
                cpu.cpuidRead.cpuidValid = true;
            }
            else
            {
                cpu.cpuidRead.source = cpuid::INVALID;
            }
        }
    }
}
static void initCPUInfo(std::vector<CPUInfo>& cpuInfo)
{
    cpuInfo.reserve(max_cpus);
    getClientAddrs(cpuInfo);
}

static void getCPUData(std::vector<CPUInfo>& cpuInfo,
                       cpuid::cpuidState cpuState)
{
    for (CPUInfo& cpu : cpuInfo)
    {
        if (!cpu.cpuidRead.cpuidValid ||
            (cpu.cpuidRead.source == cpuid::OVERWRITTEN))
        {
            getCPUID(cpu);
            parseCPUInfo(cpu, cpuState);
        }
    }
    if (cpuState == cpuid::EVENT)
    {
        overwriteCPUInfo(cpuInfo);
    }
    for (CPUInfo& cpu : cpuInfo)
    {
        if (cpu.cpuidRead.cpuidValid)
        {
            if (PECI_CC_UA(cpu.coreMaskRead.coreMaskCc))
            {
                getCoreMasks(cpuInfo);
            }
            if (PECI_CC_UA(cpu.chaCountRead.chaCountCc))
            {
                getCHACounts(cpuInfo);
            }
        }
    }
}

static std::string newTimestamp(void)
{
    char logTime[64];
    time_t curtime;
    struct tm* loctime;

    // Add the timestamp
    curtime = time(NULL);
    loctime = localtime(&curtime);
    if (NULL != loctime)
    {
        strftime(logTime, sizeof(logTime), "%FT%TZ", loctime);
    }
    return logTime;
}

static bool getCPUInfo(std::vector<CPUInfo>& cpuInfo)
{
    cpuInfo.reserve(max_cpus);
    getClientAddrs(cpuInfo);
    if (!getCPUModels(cpuInfo))
    {
        return false;
    }
    savePeciWake(cpuInfo);
    setPeciWake(cpuInfo, ON);
    if (!getCoreMasks(cpuInfo))
    {
        return false;
    }

    return getCHACounts(cpuInfo);
}

static void logTimestamp(cJSON* parent, std::string& logTime)
{
    cJSON_AddStringToObject(parent, "timestamp", logTime.c_str());
}

static void logTriggerType(cJSON* parent, const std::string& triggerType)
{
    cJSON_AddStringToObject(parent, "trigger_type", triggerType.c_str());
}

static void logPlatformName(cJSON* parent)
{
    cJSON_AddStringToObject(parent, "platform_name", getUuid().c_str());
}

static void logMetaDataCommon(cJSON* parent, std::string& logTime,
                              const std::string& triggerType)
{
    logTimestamp(parent, logTime);
    logTriggerType(parent, triggerType);
    logPlatformName(parent);
}

static cJSON*
    addSectionLog(cJSON* parent, crashdump::CPUInfo& cpuInfo,
                  std::string sectionName,
                  const std::function<int(crashdump::CPUInfo& cpuInfo, cJSON*)>
                      sectionLogFunc)
{
    CRASHDUMP_PRINT(INFO, stderr, "Logging %s on PECI address %d\n",
                    sectionName.c_str(), cpuInfo.clientAddr);

    // Create an empty JSON object for this section if it doesn't already
    // exist
    cJSON* logSectionJson;
    if ((logSectionJson = cJSON_GetObjectItemCaseSensitive(
             parent, sectionName.c_str())) == NULL)
    {
        cJSON_AddItemToObject(parent, sectionName.c_str(),
                              logSectionJson = cJSON_CreateObject());
    }

    // Get the log for this section
    int ret = 0;
    if ((ret = sectionLogFunc(cpuInfo, logSectionJson)) != 0)
    {
        CRASHDUMP_PRINT(ERR, stderr, "Error %d during %s log\n", ret,
                        sectionName.c_str());
    }

    // Check if child data is added to the JSON section
    if (logSectionJson->child == NULL)
    {
        // If there was supposed to be child data, add a failed status
        if (ret != 0)
        {
            cJSON_AddStringToObject(logSectionJson,
                                    crashdump::dbgStatusItemName,
                                    crashdump::dbgFailedStatus);
 /*       }
        else
        {
            // Otherwise delete it
            cJSON_DeleteItemFromObjectCaseSensitive(parent,
                                                    sectionName.c_str());
            logSectionJson = NULL; */
        }
    }
    return logSectionJson;
}

static cJSON* addDisableSection(cJSON* parent, std::string sectionName)
{
    cJSON* logSection = NULL;
    cJSON* logSectionJson = NULL;
    logSection = cJSON_GetObjectItemCaseSensitive(parent, sectionName.c_str());
    if (logSection == NULL)
    {
        cJSON_AddItemToObject(parent, sectionName.c_str(),
                              logSectionJson = cJSON_CreateObject());
        cJSON_AddBoolToObject(logSectionJson, RECORD_ENABLE, false);
    }
    return logSectionJson;
}

void fillSection(cJSON* cpu, CPUInfo& cpuInfo, CrashdumpSection sectionName,
                 const std::function<int(crashdump::CPUInfo& cpuInfo, cJSON*)>
                     sectionLogFunc,
                 timespec* sectionStart, char* timeStr)
{
    cJSON* logSection = NULL;

    if (CHECK_BIT(cpuInfo.sectionMask, sectionName.section))
    {
        logSection =
            addSectionLog(cpu, cpuInfo, sectionName.name, sectionLogFunc);

        // Do not log uncore version version here
        if (sectionName.section != Section::UNCORE)
        {
            logCrashdumpVersion(logSection, cpuInfo, sectionName.record_type);
        }

        logRunTime(logSection, sectionStart, timeStr);
    }
    else
    {
        addDisableSection(cpu, sectionName.name);
    }
}

void fillSectionWithChild(
    cJSON* cpu, CPUInfo& cpuInfo, CrashdumpSection sectionName,
    const std::function<int(crashdump::CPUInfo& cpuInfo, cJSON*)>
        sectionLogFunc1,
    const std::function<int(crashdump::CPUInfo& cpuInfo, cJSON*)>
        sectionLogFunc2,
    timespec* sectionStart, char* timeStr)
{
    cJSON* logSection = NULL;

    if (CHECK_BIT(cpuInfo.sectionMask, sectionName.section))
    {
        addSectionLog(cpu, cpuInfo, sectionName.name, sectionLogFunc1);
        logSection =
            addSectionLog(cpu, cpuInfo, sectionName.name, sectionLogFunc2);
        if (logSection)
        {
            logCrashdumpVersion(logSection, cpuInfo, sectionName.record_type);
            logRunTime(logSection, sectionStart, timeStr);
        }
    }
    else
    {
        addDisableSection(cpu, sectionName.name);
    }
}

void fillMetaDataCPU(cJSON* crashlogData, CPUInfo cpuInfo,
                     InputFileInfo* inputFileInfo)
{
    cJSON* logSection = NULL;
    if (CHECK_BIT(cpuInfo.sectionMask, crashdump::METADATA))
    {
        logSection =
            addSectionLog(crashlogData, cpuInfo, "METADATA", logSysInfo);

        if (!inputFileInfo->unique)
        {
            // Fill in Input File Info in METADATA cpu section
            logSysInfoInputfile(cpuInfo, logSection, inputFileInfo);
        }
    }
}

void logInputFileVersion(cJSON* root, CPUInfo cpuInfo,
                         InputFileInfo* inputFileInfo)
{
    cJSON* jsonVer = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(inputFileInfo->buffers[cpuInfo.model],
                                         "crash_data"),
        "Version");

    if ((jsonVer != NULL) && cJSON_IsString(jsonVer))
    {
        cJSON_AddStringToObject(root, "_input_file_ver", jsonVer->valuestring);
    }
    else
    {
        cJSON_AddStringToObject(root, "_input_file_ver", MD_NA);
    }
}

void fillMetaDataCommon(cJSON* metaData, CPUInfo cpuInfo,
                        InputFileInfo* inputFileInfo,
                        const std::string& triggerType, std::string& timestamp,
                        timespec* crashdumpStart, timespec* sectionStart,
                        char* timeStr)
{
    if (CHECK_BIT(cpuInfo.sectionMask, crashdump::METADATA))
    {
        // Fill in common System Info
        logSysInfoCommon(metaData);

        if (metaData != NULL)
        {
            logMetaDataCommon(metaData, timestamp, triggerType);
            logCrashdumpVersion(metaData, cpuInfo, record_type::metadata);
            logInputFileVersion(metaData, cpuInfo, inputFileInfo);
            if (inputFileInfo->unique)
            {
                logSysInfoInputfile(cpuInfo, metaData, inputFileInfo);
            }
            logRunTime(metaData, sectionStart, timeStr);
            logRunTime(metaData, crashdumpStart, "_total_time");
        }
    }
    else
    {
        cJSON_AddBoolToObject(metaData, RECORD_ENABLE, false);
    }
}

static void cleanupInputFiles(InputFileInfo* inputFileInfo)
{
    for (int i = 0; i < NUMBER_OF_CPU_MODELS; i++)
    {
        FREE(inputFileInfo->filenames[i]);
        cJSON_Delete(inputFileInfo->buffers[i]);
    }
}


#ifdef SPX_BMC_ACD
void ami_createCrashdump(std::vector<crashdump::CPUInfo>& cpuInfo,
                     std::string& crashdumpContents,
                     const std::string& triggerType, std::string& timestamp,
                     bool isTelemetry)
{
    cJSON* root = NULL;
    cJSON* crashlogData = NULL;
    cJSON* metaData = NULL;
    cJSON* processors = NULL;
    cJSON* cpu = NULL;
    cJSON* logSection = NULL;
    char* out = NULL;
//    int ret;
InputFileInfo inputFileInfo = {
        .unique = true, .filenames = {NULL}, .buffers = {NULL}};

    CRASHDUMP_PRINT(INFO, stderr, "Crashdump started...\n");
    crashdump::getCPUData(cpuInfo, cpuid::EVENT);
    crashdump::savePeciWake(cpuInfo);
    crashdump::setPeciWake(cpuInfo, ON);

/*
// Removed in ACD-0.9
    // Get the list of CPU Info for this log
    std::vector<crashdump::CPUInfo> cpuInfo;

    fprintf(stderr, "Crashdump started...\n");
    if (!crashdump::getCPUInfo(cpuInfo))
    {
        fprintf(stderr, "Failed to get CPU Info!\n");
        crashdump::checkPeciWake(cpuInfo);
        crashdump::setPeciWake(cpuInfo, OFF);
        return;
    }
*/
    loadInputFiles(cpuInfo, &inputFileInfo, isTelemetry);

    // start the JSON tree for CPU dump
    root = cJSON_CreateObject();

    // Build the CPU Crashdump JSON file
    // Everything is logged under a "crash_data" section
    cJSON_AddItemToObject(root, "crash_data",
                          crashlogData = cJSON_CreateObject());

    // Create the METADATA section
    cJSON_AddItemToObject(crashlogData, "METADATA",
                          metaData = cJSON_CreateObject());

    // Create the processors section
    cJSON_AddItemToObject(crashlogData, "PROCESSORS",
                          processors = cJSON_CreateObject());

    // Include the version field
    logCrashdumpVersion(processors, cpuInfo[0], record_type::bmcAutonomous);

    // Fill in the Crashdump data in the correct order (uncore to core) for
    // each CPU
    struct timespec sectionStart, crashdumpStart;

    clock_gettime(CLOCK_MONOTONIC, &sectionStart);
    clock_gettime(CLOCK_MONOTONIC, &crashdumpStart);

    char timeStr[] = "_time";
    for (int i = 0; i < cpuInfo.size(); i++)
    {
      for (int j= 0, address= MIN_CLIENT_ADDR; j < max_cpus && address <= MAX_CLIENT_ADDR;j++, address++)
      {
	if (peci_Ping(address) == PECI_CC_SUCCESS)
        {
        // Create a section for this cpu
        char cpuString[8];
        cd_snprintf_s(cpuString, sizeof(cpuString), "cpu%d", i);
        cJSON_AddItemToObject(processors, cpuString,
                              cpu = cJSON_CreateObject());

        // Fill in the Core Crashdump
        if (cpuInfo[i].cpuidRead.cpuidValid)
        {
            fillSection(cpu, cpuInfo[i], sectionNames[Section::UNCORE],
                        logUncoreStatus, &sectionStart, timeStr);
            fillSectionWithChild(cpu, cpuInfo[i],
                                 sectionNames[Section::BIG_CORE], logCrashdump,
                                 logSqDump, &sectionStart, timeStr);
            fillSectionWithChild(cpu, cpuInfo[i], sectionNames[Section::MCA],
                                 logCoreMca, logUncoreMca, &sectionStart,
                                 timeStr);
            fillSection(cpu, cpuInfo[i], sectionNames[Section::TOR], logTorDump,
                        &sectionStart, timeStr);
            fillSection(cpu, cpuInfo[i], sectionNames[Section::PM_INFO],
                        logPowerManagement, &sectionStart, timeStr);
            fillSection(cpu, cpuInfo[i], sectionNames[Section::ADDRESS_MAP],
                        logAddressMap, &sectionStart, timeStr);
	    #ifdef CONFIG_SPX_FEATURE_OEMDATA_SECTION
	    if((dumpSec == -1) || (dumpSec == oemData))
	    {
		    // Fill in the OEM Data
			 logSection =addSectionLog(cpu, cpuInfo[i], "oemdata", logOemData);
			 if (logSection)
			 {
				 logRunTime(logSection, &sectionStart, timeStr);
			 }
	    }
	    #endif
            fillMetaDataCPU(crashlogData, cpuInfo[i], &inputFileInfo);
	}
	}
	else
		break;
      }
  }
    fillMetaDataCommon(metaData, cpuInfo[0], &inputFileInfo, triggerType,
                       timestamp, &crashdumpStart, &sectionStart, timeStr);
/* // Removed in ACD-0.9
       if ((dumpSec == -1) || (dumpSec == bigcoreCrashdump))
        {
            // Fill in the Core Crashdump
            logSection = addSectionLog(cpu, cpuInfo[i], "big_core", logCrashdump);
            if (logSection)
            {
                // Include the version
                logCrashdumpVersion(logSection, cpuInfo[i],
                                    record_type::coreCrashLog);
            }
        }
        if ((dumpSec == -1) || (dumpSec == bigcoreSqdump))
        {
            // Fill in the SQ dump
            addSectionLog(cpu, cpuInfo[i], "big_core", logSqDump);
            logRunTime(logSection, &sectionStart, timeStr);
        }
        if ((dumpSec == -1) || (dumpSec == coreMca))
        {
            // Fill in the Core MCA data
            addSectionLog(cpu, cpuInfo[i], "MCA", logCoreMca);
        }
        if ((dumpSec == -1) || (dumpSec == uncoreMca))
        {
            // Fill in the Uncore MCA data
            logSection = addSectionLog(cpu, cpuInfo[i], "MCA", logUncoreMca);
            if (logSection)
            {
                // Include the version
                logCrashdumpVersion(logSection, cpuInfo[i], record_type::mcaLog);
                logRunTime(logSection, &sectionStart, timeStr);
            }
        }
        if ((dumpSec == -1) || (dumpSec == uncoreStatus))
        {
            // Fill in the Uncore Status
            logSection = addSectionLog(cpu, cpuInfo[i], "uncore", logUncoreStatus);
            if (logSection)
            {
                logRunTime(logSection, &sectionStart, timeStr);
            }
        }
        if ((dumpSec == -1) || (dumpSec == torDump))
        {
            // Fill in the TOR Dump
            logSection = addSectionLog(cpu, cpuInfo[i], "TOR", logTorDump);
            if (logSection)
            {
                // Include the version
                logCrashdumpVersion(logSection, cpuInfo[i], record_type::torDump);
                logRunTime(logSection, &sectionStart, timeStr);
            }
        }
        if ((dumpSec == -1) || (dumpSec == pmInfo))
        {
            // Fill in the Power Management Info
            logSection =
                addSectionLog(cpu, cpuInfo[i], "PM_info", logPowerManagement);
            if (logSection)
            {
                // Include the version
                logCrashdumpVersion(logSection, cpuInfo[i], record_type::pmInfo);
                logRunTime(logSection, &sectionStart, timeStr);
            }
        }
        if ((dumpSec == -1) || (dumpSec == addrMap))
        {
            // Fill in the Address Map
            logSection =
                addSectionLog(cpu, cpuInfo[i], "address_map", logAddressMap);
            if (logSection)
            {
                // Include the version
                logCrashdumpVersion(logSection, cpuInfo[i],
                                    record_type::addressMap);
                logRunTime(logSection, &sectionStart, timeStr);
            }
        }
#ifdef CONFIG_SPX_FEATURE_OEMDATA_SECTION
	if((dumpSec == -1) || (dumpSec == oemData))
	{
		for (int i = 0; i < cpuInfo.size(); i++)
		{
			// Fill in the OEM Data
			logSection =
    	        addSectionLog(cpu, cpuInfo[i], "oemdata", logOemData);
        	if (logSection)
	        {
    	        logRunTime(logSection, &sectionStart, timeStr);
        	}
		}
	}
#endif
//     if ((dumpSec == -1) || (dumpSec == metaData))
//      {
//          logSection =
//              addSectionLog(crashlogData, cpuInfo[i], "METADATA", logSysInfo);
//      }
//  }

    // Fill in common System Info
    logSysInfoCommon(logSection);

    if (logSection != NULL)
    {
        logTimestamp(logSection, timestamp);
        logTriggerType(logSection, triggerType);
        logPlatformName(logSection);
        logCrashdumpVersion(logSection, cpuInfo[0], record_type::metadata);
        logRunTime(logSection, &sectionStart, timeStr);
        logRunTime(logSection, &crashdumpStart, "_total_time");
    }
*/

#ifdef CONFIG_SPX_FEATURE_CRASHDUMP_PRINT_UNFORMATTED
    out = cJSON_PrintUnformatted(root);
#else
    out = cJSON_Print(root);
#endif

    if (out != NULL)
    {
        crashdumpContents = out;
        cJSON_free(out);
        fprintf(stderr, "Crashdump Completed!\n");
    }
    else
    {
        fprintf(stderr, "cJSON_Print Failed\n");
    }

    // Clear crashedCoreMask every time crashdump is run
    for (size_t i = 0; i < cpuInfo.size(); i++)
    {
	 for (int j= 0, address= MIN_CLIENT_ADDR; address <= MAX_CLIENT_ADDR;j++, address++)
	{
        if (peci_Ping(address) == PECI_CC_SUCCESS)
		cpuInfo[i].crashedCoreMask = 0;
	else
		break;
	}
   }
    cleanupInputFiles(&inputFileInfo);
    cJSON_Delete(root);
    crashdump::checkPeciWake(cpuInfo);
    crashdump::setPeciWake(cpuInfo, OFF);
	
}

static void ami_newOnDemandLog(std::vector<crashdump::CPUInfo>& cpuInfo,
					std::string& crashdumpContents,
					std::string& timestamp)
{
    constexpr char const* crashdumpFile = "crashdump.json";
    FILE* fpJson = NULL;
    struct stat st = {0};
    char out_file[512];
    char * out = NULL;

    // Start the log to the on-demand file
    ami_createCrashdump(cpuInfo, crashdumpContents, triggerTypeOnDemand, timestamp, false);
    if (crashdumpContents.empty())
    {
        // Log is empty, so don't save it
        return;
    }

    if (stat(crashdumpDir, &st) == -1) 
    {
	if (mkdir(crashdumpDir, 0700) != 0)
	{
            fprintf(stderr, "Failed to create %s; Error %s\n", crashdumpDir, strerror(errno));
	    return;
    	}
    }
    // Create the new crashdump filename
    cd_snprintf_s(out_file, sizeof(out_file), "%s/%s", crashdumpDir, crashdumpFile);
  //  fprintf(stderr, "Crashdump data is available in: %s\n", out_file);

    // open the JSON file to write the crashdump contents
    fpJson = fopen(out_file, "w");
    if (fpJson != NULL)
    {
        fprintf(fpJson, "%s", crashdumpContents.c_str());
        fclose(fpJson);
#ifdef CONFIG_SPX_FEATURE_ACD_BAFI
        // create BAFI.json using acdbafigenerator binary app
        system("/usr/local/bin/acdbafigenerator -c");
#endif
    }
	else
    {
        fprintf(stderr, "cJSON_Print Failed\n");
    }
	//compress and move crashdump files to /conf/crashdump folder
	system("/etc/init.d/crashdump_compress.sh &");
	fprintf(stderr, "Crashdump data is available in: /var/crashdump/sysdebug1.json\n");
#ifdef CONFIG_SPX_FEATURE_ACD_BAFI
	fprintf(stderr, "BAFI json is available in:/var/crashdump/json/bafi_decoded1.json\n");
#endif
}
#endif

#ifndef SPX_BMC_ACD
void createCrashdump(std::vector<crashdump::CPUInfo>& cpuInfo,
                     std::string& crashdumpContents,
                     const std::string& triggerType, std::string& timestamp,
                     bool isTelemetry)
{
    cJSON* root = NULL;
    cJSON* crashlogData = NULL;
    cJSON* metaData = NULL;
    cJSON* processors = NULL;
    cJSON* cpu = NULL;
    cJSON* logSection = NULL;
    char* out = NULL;
    InputFileInfo inputFileInfo = {
        .unique = true, .filenames = {NULL}, .buffers = {NULL}};
//    int ret;
// Removed in ACD_0.9
    // Get the list of CPU Info for this log
    std::vector<crashdump::CPUInfo> cpuInfo;

    fprintf(stderr, "Crashdump started...\n");
    if (!crashdump::getCPUInfo(cpuInfo))
    {
        fprintf(stderr, "Failed to get CPU Info!\n");
        crashdump::checkPeciWake(cpuInfo);
        crashdump::setPeciWake(cpuInfo, OFF)                                                                               ;
        return;
    }

    CRASHDUMP_PRINT(INFO, stderr, "Crashdump started...\n");
    crashdump::getCPUData(cpuInfo, cpuid::EVENT);
    crashdump::savePeciWake(cpuInfo);
    crashdump::setPeciWake(cpuInfo, ON);

    loadInputFiles(cpuInfo, &inputFileInfo, isTelemetry);

    // start the JSON tree for CPU dump
    root = cJSON_CreateObject();

    // Build the CPU Crashdump JSON file
    // Everything is logged under a "crash_data" section
    cJSON_AddItemToObject(root, "crash_data",
                          crashlogData = cJSON_CreateObject());

    // Create the METADATA section
    cJSON_AddItemToObject(crashlogData, "METADATA",
                          metaData = cJSON_CreateObject());

    // Create the processors section
    cJSON_AddItemToObject(crashlogData, "PROCESSORS",
                          processors = cJSON_CreateObject());

    // Include the version field
    logCrashdumpVersion(processors, cpuInfo[0], record_type::bmcAutonomous);

    // Fill in the Crashdump data in the correct order (uncore to core) for
    // each CPU
    struct timespec sectionStart, crashdumpStart;

    clock_gettime(CLOCK_MONOTONIC, &sectionStart);
    clock_gettime(CLOCK_MONOTONIC, &crashdumpStart);

    char timeStr[] = "_time";
//    for (int i = 0; i < cpuInfo.size(); i++)
    for (size_t i = 0; i < cpuInfo.size(); i++)
    {
        // Create a section for this cpu
        char cpuString[8];
        cd_snprintf_s(cpuString, sizeof(cpuString), "cpu%d", i);
        cJSON_AddItemToObject(processors, cpuString,
                              cpu = cJSON_CreateObject());

        // Fill in the Core Crashdump
        logSection = addSectionLog(cpu, cpuInfo[i], "big_core", logCrashdump);
        if (logSection)
        {
            // Include the version
            logCrashdumpVersion(logSection, cpuInfo[i],
                                record_type::coreCrashLog);
        }

        // Fill in the SQ dump
        addSectionLog(cpu, cpuInfo[i], "big_core", logSqDump);
        logRunTime(logSection, &sectionStart, timeStr);

        // Fill in the Core MCA data
        addSectionLog(cpu, cpuInfo[i], "MCA", logCoreMca);

        // Fill in the Uncore MCA data
        logSection = addSectionLog(cpu, cpuInfo[i], "MCA", logUncoreMca);
        if (logSection)
        {
            // Include the version
            logCrashdumpVersion(logSection, cpuInfo[i], record_type::mcaLog);
            logRunTime(logSection, &sectionStart, timeStr);
        }

        // Fill in the Uncore Status
        logSection = addSectionLog(cpu, cpuInfo[i], "uncore", logUncoreStatus);
        if (logSection)
        {
            logRunTime(logSection, &sectionStart, timeStr);
        }

        // Fill in the TOR Dump
        logSection = addSectionLog(cpu, cpuInfo[i], "TOR", logTorDump);
        if (logSection)
        {
            // Include the version
            logCrashdumpVersion(logSection, cpuInfo[i], record_type::torDump);
            logRunTime(logSection, &sectionStart, timeStr);
        }

        // Fill in the Power Management Info
        logSection =
            addSectionLog(cpu, cpuInfo[i], "PM_info", logPowerManagement);
        if (logSection)
        {
            // Include the version
            logCrashdumpVersion(logSection, cpuInfo[i], record_type::pmInfo);
            logRunTime(logSection, &sectionStart, timeStr);
        }

        // Fill in the Address Map
        logSection =
            addSectionLog(cpu, cpuInfo[i], "address_map", logAddressMap);
        if (logSection)
        {
            // Include the version
            logCrashdumpVersion(logSection, cpuInfo[i],
                                record_type::addressMap);
            logRunTime(logSection, &sectionStart, timeStr);
        }
        logSection =
            addSectionLog(crashlogData, cpuInfo[i], "METADATA", logSysInfo);
    }

    // Fill in common System Info
    logSysInfoCommon(logSection);

    if (logSection != NULL)
    {
        logTimestamp(logSection, timestamp);
        logTriggerType(logSection, triggerType);
        logPlatformName(logSection);
        logCrashdumpVersion(logSection, cpuInfo[0], record_type::metadata);
        logRunTime(logSection, &sectionStart, timeStr);
        logRunTime(logSection, &crashdumpStart, "_total_time");
    }
#ifdef OEMDATA_SECTION
    // OEM Customer Section
    clock_gettime(CLOCK_MONOTONIC, &sectionStart);
    crashdumpStart.tv_sec = sectionStart.tv_sec;
    crashdumpStart.tv_nsec = sectionStart.tv_nsec;
    cJSON* oemCrashlogData = NULL;
//    crashdumpStart.tv_nsec = sectionStart.tv_nsec cJSON* oemCrashlogData = NULL;
    cJSON* oemProcessors = NULL;
    cJSON* oemCpu = NULL;
    cJSON* oemLogSection = NULL;
    cJSON_AddItemToObject(root, "oemdata",
                          oemCrashlogData = cJSON_CreateObject());
    cJSON_AddItemToObject(oemCrashlogData, "PROCESSORS",
                          oemProcessors = cJSON_CreateObject());
    logCrashdumpVersion(oemProcessors, cpuInfo[0], record_type::bmcAutonomous);

    for (int i = 0; i < cpuInfo.size(); i++)
    {
        char cpuString[8];
        cd_snprintf_s(cpuString, sizeof(cpuString), "cpu%d", i);
        cJSON_AddItemToObject(oemProcessors, cpuString,
                              oemCpu = cJSON_CreateObject());
        oemLogSection =
            addSectionLog(oemCpu, cpuInfo[i], "oemdata", logOemData);
        if (oemLogSection)
        {
            logRunTime(oemLogSection, &sectionStart, timeStr);
        }
    }
    if (oemLogSection != NULL)
    {
        logTimestamp(oemCrashlogData, timestamp);
        logRunTime(oemCrashlogData, &crashdumpStart, "_total_time");
    }
#endif
#ifdef CRASHDUMP_PRINT_UNFORMATTED
    out = cJSON_PrintUnformatted(root);
#else
    out = cJSON_Print(root);
#endif
    if (out != NULL)
    {
        crashdumpContents = out;
        cJSON_free(out);
        CRASHDUMP_PRINT(INFO, stderr, "Completed!\n");
    }
    else
    {
        CRASHDUMP_PRINT(ERR, stderr, "cJSON_Print Failed\n");
    }

    // Clear crashedCoreMask every time crashdump is run
    for (size_t i = 0; i < cpuInfo.size(); i++)
    {
        cpuInfo[i].crashedCoreMask = 0;
    }

    cleanupInputFiles(&inputFileInfo);
    cJSON_Delete(root);
    crashdump::checkPeciWake(cpuInfo);
    crashdump::setPeciWake(cpuInfo, OFF);
}

static int scandir_filter(const struct dirent* dirEntry)
{
    // Filter for just the crashdump files
    return (strncmp(dirEntry->d_name, crashdumpPrefix.c_str(),
                    crashdumpPrefix.size()) == 0);
}

static void dbusRemoveOnDemandLog()
{
    // Always make sure the D-Bus properties are removed
    server->remove_interface(logIface);
    logIface.reset();

    std::error_code ec;
    if (!std::filesystem::exists(crashdumpDir))
    {
        // Can't delete something that doesn't exist
        return;
    }

    for (auto& fileList : std::filesystem::directory_iterator(crashdumpDir))
    {
        // always iterate through all files in the directory to clear
        // on-demand files. This is just a safeguard in the event more
        // than a single file is created.
        std::string fname = fileList.path();
        if (fname.substr(crashdumpDir.string().size() + 1,
                         crashdumpFileRoot.size()) == crashdumpFileRoot)
        {
            if (!(std::filesystem::remove(fname, ec)))
            {
                CRASHDUMP_PRINT(ERR, stderr, "failed to remove %s: %s\n",
                                fname.c_str(), ec.message().c_str());
                break;
            }
        }
    }
}

static void dbusRemoveTelemetryLog()
{
    // Always make sure the D-Bus properties are removed
    server->remove_interface(logIface);
    logIface.reset();

    std::error_code ec;
    if (!std::filesystem::exists(crashdumpDir))
    {
        // Can't delete something that doesn't exist
        return;
    }

    for (auto& fileList : std::filesystem::directory_iterator(crashdumpDir))
    {
        // always iterate through all files in the directory to clear
        // on-demand files. This is just a safeguard in the event more
        // than a single file is created.
        std::string fname = fileList.path();
        if (fname.substr(crashdumpDir.string().size() + 1,
                         crashdumpTelemetryFileRoot.size()) ==
            crashdumpTelemetryFileRoot)
        {
            if (!(std::filesystem::remove(fname, ec)))
            {
                fprintf(stderr, "failed to remove %s: %s\n", fname.c_str(),
                        ec.message().c_str());
                break;
            }
        }
    }
}

static void dbusAddLog(const std::string& logContents,
                       const std::string& timestamp,
                       const std::string& dbusPath, const std::string& filename)
{
    FILE* fpJson = NULL;
    std::error_code ec;
    std::filesystem::path out_file = crashdumpDir / filename;

    // create the crashdump/output directory if it doesn't exist
    if (!(std::filesystem::create_directories(crashdumpDir, ec)))
    {
        if (ec.value() != 0)
        {
            CRASHDUMP_PRINT(ERR, stderr, "failed to create %s: %s\n",
                            crashdumpDir.c_str(), ec.message().c_str());
            return;
        }
    }

    fpJson = fopen(out_file.c_str(), "w");
    if (fpJson)
    {
        fprintf(fpJson, "%s", logContents.c_str());
        fclose(fpJson);
    }
    logIface = server->add_interface(dbusPath, crashdumpInterface);
    logIface->register_property("Log", out_file.string());
    logIface->register_property("Timestamp", timestamp);
    logIface->register_property("Filename", filename);
    logIface->initialize();
}

static void newOnDemandLog(std::vector<crashdump::CPUInfo>& cpuInfo,
                           std::string& onDemandLogContents,
                           std::string& timestamp)
{
    // Start the log to the on-demand location
    createCrashdump(cpuInfo, onDemandLogContents, triggerTypeOnDemand,
                    timestamp, false);
}

static void newTelemetryLog(std::vector<crashdump::CPUInfo>& cpuInfo,
                            std::string& telemetryLogContents,
                            std::string& timestamp)
{
    // Start the log to the telemetry location
    createCrashdump(cpuInfo, telemetryLogContents, triggerTypeOnDemand,
                    timestamp, true);
}

static void incrementCrashdumpCount()
{
    // Get the current count
    conn->async_method_call(
        [](boost::system::error_code ec,
           const std::variant<uint8_t>& property) {
            if (ec)
            {
                CRASHDUMP_PRINT(ERR, stderr, "Failed to get Crashdump count\n");
                return;
            }
            const uint8_t* crashdumpCountVariant =
                std::get_if<uint8_t>(&property);
            if (crashdumpCountVariant == nullptr)
            {
                CRASHDUMP_PRINT(ERR, stderr,
                                "Unable to read Crashdump count\n");
                return;
            }
            uint8_t crashdumpCount = *crashdumpCountVariant;
            if (crashdumpCount == std::numeric_limits<uint8_t>::max())
            {
                CRASHDUMP_PRINT(ERR, stderr,
                                "Maximum crashdump count reached\n");
                return;
            }
            // Increment the count
            crashdumpCount++;
            conn->async_method_call(
                [](boost::system::error_code ec) {
                    if (ec)
                    {
                        CRASHDUMP_PRINT(ERR, stderr,
                                        "Failed to set Crashdump count\n");
                    }
                },
                "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/control/processor_error_config",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Control.Processor.ErrConfig",
                "CrashdumpCount", std::variant<uint8_t>{crashdumpCount});
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/processor_error_config",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Processor.ErrConfig", "CrashdumpCount");
}

constexpr int numStoredLogs = 3;
static void dbusAddStoredLog(const std::string& storedLogContents,
                             const std::string& timestamp)
{
    constexpr char const* crashdumpFile = "crashdump_%llu";
    uint64_t crashdump_num = 0;
    struct dirent** namelist = NULL;
    FILE* fpJson = NULL;
    std::error_code ec;

    // create the crashdump/output directory if it doesn't exist
    if (!(std::filesystem::create_directories(crashdumpDir, ec)))
    {
        if (ec.value() != 0)
        {
            CRASHDUMP_PRINT(ERR, stderr, "failed to create %s: %s\n",
                            crashdumpDir.c_str(), ec.message().c_str());
            return;
        }
    }

    // Search the crashdump/output directory for existing log files
    int numLogFiles =
        scandir(crashdumpDir.c_str(), &namelist, scandir_filter, versionsort);
    if (numLogFiles < 0)
    {
        // scandir failed, so print the error
        perror("scandir");
        return;
    }

    // Get the number for this crashdump. The crashdump_num is not
    // kept as a static in order to cover crashdump service
    // restarts. In that case it is undesirable to restart the number
    // at zero.
    if (numLogFiles > 0)
    {
        // otherwise, get the number of the last log and increment it
        sscanf_s(namelist[numLogFiles - 1]->d_name, crashdumpFile,
                 &crashdump_num);
        crashdump_num++;
    }

    // In case multiple crashdumps are triggered for the same error, the policy
    // is to keep the first log until it is manually cleared and rotate through
    // additional logs.  This guarantees that we have the first and last log of
    // a failure.
    for (int i = 0; i < numLogFiles; i++)
    {
        // Except for log 0, if it's below the number of saved logs, delete it
        if ((i != 0) && (i <= (numLogFiles - (numStoredLogs - 1))))
        {
            std::error_code ec;
            if (!(std::filesystem::remove(crashdumpDir / namelist[i]->d_name,
                                          ec)))
            {
                CRASHDUMP_PRINT(ERR, stderr, "failed to remove %s: %s\n",
                                namelist[i]->d_name, ec.message().c_str());
            }
            // Now remove the interface for the deleted log
            auto eraseit =
                std::find_if(storedLogIfaces.begin(), storedLogIfaces.end(),
                             [&namelist, &i](auto& log) {
                                 std::string& storedName = std::get<0>(log);
                                 return (storedName == namelist[1]->d_name);
                             });

            if (eraseit != std::end(storedLogIfaces))
            {
                crashdump::server->remove_interface(std::get<1>(*eraseit));
                storedLogIfaces.erase(eraseit);
            }
        }
        // Free the file data
        free(namelist[i]);
    }
    // Free the scandir data
    free(namelist);

    // Create the new crashdump filename
    std::string new_logfile_name = crashdumpPrefix +
                                   std::to_string(crashdump_num) + "-" +
                                   timestamp + ".json";
    std::filesystem::path out_file = crashdumpDir / new_logfile_name;

    // open the JSON file to write the crashdump contents
    fpJson = fopen(out_file.c_str(), "w");
    if (fpJson != NULL)
    {
        fprintf(fpJson, "%s", storedLogContents.c_str());
        fclose(fpJson);
    }

    // Add the new interface for this log
    std::filesystem::path path =
        std::filesystem::path(crashdumpPath) / std::to_string(crashdump_num);
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceLog =
        server->add_interface(path.c_str(), crashdumpInterface);
    storedLogIfaces.emplace_back(new_logfile_name, ifaceLog);
    // Log Property

    ifaceLog->register_property("Log", out_file.string());
    ifaceLog->register_property("Timestamp", timestamp);
    ifaceLog->register_property("Filename", new_logfile_name);
    ifaceLog->initialize();

    // Increment the count for this completed crashdump
    incrementCrashdumpCount();
}

static void newStoredLog(std::vector<crashdump::CPUInfo>& cpuInfo,
                         std::string& storedLogContents,
                         const std::string& triggerType, std::string& timestamp)
{
    // Start the log
    createCrashdump(cpuInfo, storedLogContents, triggerType, timestamp, false);
}

static bool isPECIAvailable()
{
    std::vector<CPUInfo> cpuInfo;
    getClientAddrs(cpuInfo);
    if (cpuInfo.empty())
    {
        CRASHDUMP_PRINT(ERR, stderr, "PECI is not available!\n");
        return false;
    }
    return true;
}

/** Exception for when a log is attempted while power is off. */
struct PowerOffException final : public sdbusplus::exception_t
{
    const char* name() const noexcept override
    {
        return "org.freedesktop.DBus.Error.NotSupported";
    };
    const char* description() const noexcept override
    {
        return "Power off, cannot access peci";
    };
    const char* what() const noexcept override
    {
        return "org.freedesktop.DBus.Error.NotSupported: "
               "Power off, cannot access peci";
    };
};
/** Exception for when a log is attempted while another is in progress. */
struct LogInProgressException final : public sdbusplus::exception_t
{
    const char* name() const noexcept override
    {
        return "org.freedesktop.DBus.Error.ObjectPathInUse";
    };
    const char* description() const noexcept override
    {
        return "Log in progress";
    };
    const char* what() const noexcept override
    {
        return "org.freedesktop.DBus.Error.ObjectPathInUse: "
               "Log in progress";
    };
};
#endif
} // namespace crashdump

#ifdef SPX_BMC_ACD
static int IsValidError()
{
    uint32_t u32PeciData;
    uint8_t u8Cpu;
    uint8_t cc = 0;
    int (*IsError)(uint32_t);

    fprintf(stderr, "Crashdump - Read MCA_ERR_SRC_LOG\n");

    // Get the list of CPU Info for this log
    std::vector<crashdump::CPUInfo> cpuInfo;
    if (!crashdump::getCPUInfo(cpuInfo))
    {
        fprintf(stderr, "Failed to get CPU Info!\n");
        return 0;
    }

    for (u8Cpu = 0; u8Cpu < cpuInfo.size(); u8Cpu++)
    {
        memset(&u32PeciData, 0x0, sizeof(uint32_t));
        if (peci_RdPkgConfig(cpuInfo[u8Cpu].clientAddr, PECI_MBX_INDEX_CPU_ID,
      	                 PECI_PKG_ID_MACHINE_CHECK_STATUS, sizeof(uint32_t),
               	         (uint8_t*)&u32PeciData, &cc) != PECI_CC_SUCCESS)
        {
            fprintf(stderr, "[DBG] - CPU #%d MCA_ERR_SRC_LOG Failed\n", u8Cpu);
            return 0;
        }
        printf("MCA_ERR_SRC_LOG regsiter = 0x%x, cpu = %d\n", u32PeciData, u8Cpu);

        IsError = (int (*)(uint32_t))dlsym(dl_handle,"ACDPDK_IsValidError");
        if (IsError != NULL)
        {
            if (IsError(u32PeciData))
                return 1;
        }
        else
            return 1;
    }

    return 0;
}
#endif

int main(int argc, char* argv[])
{
#ifdef SPX_BMC_ACD
    std::string ami_onDemandLogContents;
	std::string onDemandTimestamp = crashdump::newTimestamp();
    int (*IsACDTriggered)();
    int (*IsValidTrigger)();
    int (*PostDumpAction)();
    int validTrigger = 0, checkErrCount = 0;

    dl_handle = dlopen((char *)ACDPDK_LIB,RTLD_NOW);
    if(!dl_handle)
    {
        printf("Error in loading ACDPDK_LIB library %s\n",dlerror());
        return 1;
    }

    /* Check only if one instance of Crashdump running */
    only_one_instance((char *)crashdump::crashdumpDir);

    /* Process arguments */
    parse_arguments(argc, argv);
/*Getting Max CPUS*/
   std::vector<crashdump::CPUInfo> cpuInfo;

    crashdump::getCPUInfo(cpuInfo);
    cpuInfo.reserve(max_cpus);
    getClientAddrs(cpuInfo);
    maxCPUS(cpuInfo);


	crashdump::initCPUInfo(crashdump::cpuInfo);
	crashdump::getCPUData(crashdump::cpuInfo, cpuid::STARTUP);

    if (dumpNow == 1)
    {
    
        crashdump::ami_newOnDemandLog(crashdump::cpuInfo, ami_onDemandLogContents, onDemandTimestamp);
        return 0;
    }
 
    IsACDTriggered = (int (*)())dlsym(dl_handle,"ACDPDK_IsACDTriggered");
    if (IsACDTriggered == NULL)
    {
        dlclose(dl_handle);
        return 1;
    }

    while(1) 
    {
        if (IsACDTriggered() != 1)
        {
            dlclose(dl_handle);
            return 1;
        }

        validTrigger = 1;
        checkErrCount = 0;
        while (!IsValidError())
        {
            //Check for error for about 30 seconds (delay of 100ms * 300 iterations)
            if (checkErrCount >= ERRCHECKMAXCOUNT)
            {
                fprintf(stderr, "CRASHDUMP error is not valid!\n");
                break;
            }

            IsValidTrigger = (int (*)())dlsym(dl_handle,"ACDPDK_IsValidTrigger");
            if (IsValidTrigger != NULL)
            {
                //If not a valid CATERR, continue to wait for next valid CATERR
                if(!IsValidTrigger())
                {
                    validTrigger = 0;
                    break;
                }
                checkErrCount++;
                usleep(100000);
            }
            else
                break;
        }
        if ((validTrigger == 0) || (checkErrCount >= ERRCHECKMAXCOUNT))
            continue;

        crashdump::ami_newOnDemandLog(crashdump::cpuInfo, ami_onDemandLogContents, onDemandTimestamp);

        /* PDK for post crasdump actions */
        PostDumpAction = (int (*)())dlsym(dl_handle,"ACDPDK_PostDumpAction");
        if (PostDumpAction != NULL)
        {
            PostDumpAction();
        }
    }
#else
    // future to use for long-running tasks
    std::future<void> future;

    // setup connection to dbus
    crashdump::conn =
        std::make_shared<sdbusplus::asio::connection>(crashdump::io);
// Removed in ACD-0.9
//    std::string onDemandLogContents;

    // CPU Debug Log Object
    crashdump::conn->request_name(crashdump::crashdumpService);
    crashdump::server =
        std::make_shared<sdbusplus::asio::object_server>(crashdump::conn);

    // Reserve space for the stored log interfaces
    crashdump::storedLogIfaces.reserve(crashdump::numStoredLogs);

    // Stored Log Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceStored =
        crashdump::server->add_interface(crashdump::crashdumpPath,
                                         crashdump::crashdumpStoredInterface);

    if (!crashdump::isPECIAvailable())
    {
        CRASHDUMP_PRINT(ERR, stderr, "PECI not available\n");
    }
    crashdump::initCPUInfo(crashdump::cpuInfo);
    crashdump::getCPUData(crashdump::cpuInfo, cpuid::STARTUP);

    // Generate a Stored Log
    ifaceStored->register_method(
        "GenerateStoredLog", [&future](const std::string& triggerType) {
            if (!crashdump::isPECIAvailable())
            {
                throw crashdump::PowerOffException();
            }
            if (future.valid() && future.wait_for(std::chrono::seconds(0)) !=
                                      std::future_status::ready)
            {
                throw crashdump::LogInProgressException();
            }
            future = std::async(std::launch::async, [triggerType]() {
                std::string storedLogContents;
                std::string storedLogTime = crashdump::newTimestamp();
                crashdump::newStoredLog(crashdump::cpuInfo, storedLogContents,
                                        triggerType, storedLogTime);
                if (storedLogContents.empty())
                {
                    // Log is empty, so don't save it
                    return;
                }
                boost::asio::post(
                    crashdump::io,
                    [storedLogContents = std::move(storedLogContents),
                     storedLogTime = std::move(storedLogTime)]() mutable {
                        crashdump::dbusAddStoredLog(storedLogContents,
                                                    storedLogTime);
                    });
            });
            return std::string("Log Started");
        });
    ifaceStored->initialize();

    // DeleteAll Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceDeleteAll =
        crashdump::server->add_interface(
            crashdump::crashdumpPath, crashdump::crashdumpDeleteAllInterface);

    // Delete all stored logs
    ifaceDeleteAll->register_method("DeleteAll", []() {
        std::error_code ec;
        for (auto& [file, interface] : crashdump::storedLogIfaces)
        {
            if (!(std::filesystem::remove(crashdump::crashdumpDir / file, ec)))
            {
                CRASHDUMP_PRINT(ERR, stderr, "failed to remove %s: %s\n",
                                file.c_str(), ec.message().c_str());
            }
            crashdump::server->remove_interface(interface);
        }
        crashdump::storedLogIfaces.clear();
        crashdump::dbusRemoveOnDemandLog();
        CRASHDUMP_PRINT(INFO, stderr, "Crashdump logs cleared\n");
        return std::string("Logs Cleared");
    });
    ifaceDeleteAll->initialize();

    // OnDemand Log Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceOnDemand =
        crashdump::server->add_interface(crashdump::crashdumpPath,
                                         crashdump::crashdumpOnDemandInterface);

    // Generate an OnDemand Log
    ifaceOnDemand->register_method("GenerateOnDemandLog", [&future]() {
        if (!crashdump::isPECIAvailable())
        {
            throw crashdump::PowerOffException();
        }
        // Check if a Log is in progress
        if (future.valid() && future.wait_for(std::chrono::seconds(0)) !=
                                  std::future_status::ready)
        {
            throw crashdump::LogInProgressException();
        }
        // Remove the old on-demand log
        crashdump::dbusRemoveOnDemandLog();

        // Start the log asynchronously since it can take a long time
        future = std::async(std::launch::async, []() {
            std::string onDemandLogContents;
            std::string onDemandTimestamp = crashdump::newTimestamp();
            std::string filename =
                crashdump::crashdumpFileRoot + onDemandTimestamp + ".json";
            crashdump::newOnDemandLog(crashdump::cpuInfo, onDemandLogContents,
                                      onDemandTimestamp);
            boost::asio::post(
                crashdump::io,
                [onDemandLogContents = std::move(onDemandLogContents),
                 onDemandTimestamp = std::move(onDemandTimestamp),
                 filename]() mutable {
                    crashdump::dbusAddLog(
                        onDemandLogContents, onDemandTimestamp,
                        crashdump::crashdumpOnDemandPath, filename);
                });
        });

        // Return success
        return std::string("Log Started");
    });

    ifaceOnDemand->initialize();

    // Telemetry Log Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceTelemetry =
        crashdump::server->add_interface(
            crashdump::crashdumpPath, crashdump::crashdumpTelemetryInterface);

    // Generate a Telemetry Log
    ifaceTelemetry->register_method("GenerateTelemetryLog", [&future]() {
        if (!crashdump::isPECIAvailable())
        {
            throw crashdump::PowerOffException();
        }
        // Check if a Log is in progress
        if (future.valid() && future.wait_for(std::chrono::seconds(0)) !=
                                  std::future_status::ready)
        {
            throw crashdump::LogInProgressException();
        }
        crashdump::dbusRemoveTelemetryLog();

        // Start the log asynchronously since it can take a long time
        future = std::async(std::launch::async, []() {
            std::string telemetryLogContents;
            std::string telemetryTimestamp = crashdump::newTimestamp();
            std::string filename = crashdump::crashdumpTelemetryFileRoot +
                                   telemetryTimestamp + ".json";
            crashdump::newTelemetryLog(crashdump::cpuInfo, telemetryLogContents,
                                       telemetryTimestamp);
            boost::asio::post(
                crashdump::io,
                [telemetryLogContents = std::move(telemetryLogContents),
                 telemetryTimestamp = std::move(telemetryTimestamp),
                 filename]() mutable {
                    crashdump::dbusAddLog(
                        telemetryLogContents, telemetryTimestamp,
                        crashdump::crashdumpTelemetryPath, filename);
                });
        });

        // Return success
        return std::string("Log Started");
    });

    ifaceTelemetry->initialize();

    // Build up paths for any existing stored logs
    if (std::filesystem::exists(crashdump::crashdumpDir))
    {
        std::regex search("crashdump_([[:digit:]]+)-([[:graph:]]+?).json");
        std::smatch match;
        for (auto& p :
             std::filesystem::directory_iterator(crashdump::crashdumpDir))
        {
            std::string file = p.path().filename();
            if (std::regex_match(file, match, search))
            {
                // Log Interface
                std::filesystem::path path =
                    std::filesystem::path(crashdump::crashdumpPath) /
                    match.str(1);
                std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceLog =
                    crashdump::server->add_interface(
                        path.c_str(), crashdump::crashdumpInterface);
                crashdump::storedLogIfaces.emplace_back(file, ifaceLog);
                // Log Property
                ifaceLog->register_property("Log", p.path().string());
                ifaceLog->register_property("Timestamp", match.str(2));
                ifaceLog->register_property("Filename", file);
                ifaceLog->initialize();
            }
        }
        crashdump::dbusRemoveOnDemandLog();
    }

    // Send Raw PECI Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceRawPeci =
        crashdump::server->add_interface(crashdump::crashdumpPath,
                                         crashdump::crashdumpRawPeciInterface);

    // Send a Raw PECI command
    ifaceRawPeci->register_method(
        "SendRawPeci", [](const std::vector<std::vector<uint8_t>>& rawCmds) {
            std::vector<std::vector<uint8_t>> rawResp;
            for (auto const& rawCmd : rawCmds)
            {
                if (rawCmd.size() < 3)
                {
                    throw std::invalid_argument("Command Length too short");
                }
                std::vector<uint8_t> resp(rawCmd[2]);
                EPECIStatus rc = peci_raw(rawCmd[0], rawCmd[2], &rawCmd[3],
                                          rawCmd[1], resp.data(), resp.size());
                if (rc == PECI_CC_SUCCESS || rc == PECI_CC_TIMEOUT)
                {
                    rawResp.push_back(resp);
                }
                else
                {
                    rawResp.push_back({0});
                }
            }
            return rawResp;
        });
    ifaceRawPeci->initialize();

    crashdump::io.run();
#endif

    return 0;
}
