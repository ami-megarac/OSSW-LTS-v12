/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and
 * your use of them is governed by the express license under which they were
 * provided to you ("License"). Unless the License provides otherwise, you may
 * not use, modify, copy, publish, distribute, disclose or transmit this
 * software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in
 * the License.
 *
 ******************************************************************************/

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "libacdbafi.hpp"
#include "mca.hpp"
#include "nlohmann/json.hpp"
#include "pcilookup.hpp"
#include "report_cpx.hpp"
#include "report_icx.hpp"
#include "report_skx.hpp"
#include "summary.hpp"
#include "tor.hpp"
#include "tor_purley.hpp"
#include "tor_icx.hpp"
#include "tor_cpx.hpp"
#include "tor_skx.hpp"
#include "utils.hpp"

//static const char* crashdumpDir = "/var/crashdump/json";
constexpr char const* crashdumpFile = "/var/crashdump/json/crashdump.json";
constexpr char* memorymapFile = "/conf/crashdump/input/default_memory_map.json";

void printHelp()
{
  std::cerr << "\nNote: Make sure that the crashdump is created or\n";
  std::cerr << "have the crashdump.json file at /var/crashdump/json location before using acdbafigenerator\n";
  std::cerr << "\nUsage:\n";
  std::cerr << "---------\n";
  std::cerr << "\nacdbafigenerator [-?][-v][-m memory_map_file]\n\n";
  std::cerr << "Options:\n";
  std::cerr << "  -m\t\t\timport memory map from json file\n";
  std::cerr << "  --memory-map\t\timport memory map from json file\n";
  std::cerr << "  memory_map_file\tJSON file containing memory map ";
  std::cerr << "information. Should be named crashdump.json\n";
  //std::cerr << "  crashdump_file\tJSON file containing Autonomous Crashdump";
  std::cerr << "  -?\t\t\tUsage\n";
  std::cerr << "  --help\t\tUsage\n";
  std::cerr << "  -v\t\t\tversion information\n";
  std::cerr << "  --version\t\tversion information";
  std::cerr << "\n\nInstructions:\n";
  std::cerr << "  Step 1: In Summary TSC section, locate which TSC register has";
  std::cerr << " \"Occured first between all TSCs\" tag.\n";
  std::cerr << "  Step 2: From the socket in step 1, locate the IP and MCE ";
  std::cerr << "bank number that asserted FirstIERR and FirstMCERR (i.e. ";
  std::cerr << "FirstMCERR=CHA1 bank10, M2MEM 0 bank 12, UPI 0 bank 5).\n";
  std::cerr << "  Step 3: Correlate above MCE bank number with error ";
  std::cerr << "information from MCE section or find device Bus/Device/";
  std::cerr << "Function number in 'First.MCERR' section if available.\n\n";
  std::cerr << "I have cscript memory map created using pci.resource_check() ";
  std::cerr << "command. Can I use it in BAFI?\n";
  std::cerr << "  Yes. You can use tools/cscript_map_converter.py like this:\n\n";
  std::cerr << "     > python3 csript_map_converter.py ";
  std::cerr << "<cscript_memory_map_file_format>\n\n";
  std::cerr << "  This command will create default_memory_map.json that can be ";
  std::cerr << "later used in BAFI using -m (--memory-map) option or by\n ";
  std::cerr << "copying this map to /conf/crashdump/input/ location .";
  std::cerr << "\n\n";
}

int main(int argc, char* argv[])
//int main()
{
	
    std::cout<<"\n**ACD BAFI GENERATOR 0.95**"<<std::endl;
    auto start = std::chrono::steady_clock::now();
  uint8_t binaryFileArgumentPosition = 0;
  uint8_t crashdumpFileArgumentPosition = 1;
  uint8_t memoryMapFileArgumentPosition = 2;

  if (argc > 3 ||argc < 2)
    {
        printHelp();
        return -1;
    }

   if (std::string(argv[1]) == "-?" || std::string(argv[1]) == "--help") {
        printHelp();
        return -1;

    }
  if (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version") {
      std::cout << "BAFI version 0.95" << std::endl;
      return 0;
  }

  if (std::string(argv[1]) == "-m" || std::string(argv[1]) == "--memory-map") {
    crashdumpFileArgumentPosition = 3;
    PciBdfLookup PciBdfLookup(argv[memoryMapFileArgumentPosition]);
  } else {
//    PciBdfLookup PciBdfLookup(argv[binaryFileArgumentPosition], true);
      PciBdfLookup PciBdfLookup(memorymapFile);
  }

  if (PciBdfLookup::memorymapExistence == false) {
      printHelp();
      return -2;
  }

// Open File
//    std::ifstream i(argv[1]);
if (std::string(argv[1]) == "-m" || std::string(argv[1]) == "-c"){
    std::ifstream i(crashdumpFile);
    if (!i.good())
    {
//        std::cerr << "File '" << argv[1] << "' not found.\n";
        std::cerr << "File '" << crashdumpFile << "' not found.\n";
        printHelp();
        return -3;
    }
    // Read and check proper JSON format
    json inputData = json::parse(i, nullptr, false);
    if (inputData.is_discarded())
    {
        //std::cerr << "File '" << argv[1] << "' is not a valid JSON file\n";
        std::cerr << "File '" << crashdumpFile << "' is not a valid JSON file\n";
        return -4;
    }
    // If crash_data is a root node take only inside of crash_data node
    auto input = getProperRootNode(inputData);
    if (!input)
    {
        std::cerr << "Incorrect JSON root node. Expected crash_data or no root "
                     "node.\n";
        return -5;
    }

/*
  // Open File
  std::ifstream i(argv[crashdumpFileArgumentPosition]);
  if (!i.good()) {
    std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
              << "' not found.\n";
    printHelp();
    return -3;
  }

  // Read and check proper JSON format
  json inputData = json::parse(i, nullptr, false);

  if (inputData.is_discarded()) {
    std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
              << "' is not a valid JSON file\n";
    return -4;
  }

  // Identify CPU type - ICX for Ice Lake, CPX for Cooper Lake and SKX for Sky Lake
  // If crash_data is a root node take only inside of crash_data node
    auto input = getProperRootNode(inputData);

    if (!input) {
      std::cerr << "Incorrect JSON root node. Expected crash_data or no root "
                   "node.\n";
      return -5;
    }
*/
  // Create summary object of all data in input JSON
  Summary summary;

  summary.cpuType = getCpuId(*input);

  if (summary.cpuType == "ICX") {

    // Gather CPUs from METADATA section
    std::vector<std::string> cpus = findCpus(*input);
    if (cpus.size() == 0) {
//      std::cerr << "File '" << argv[crashdumpFileArgumentPosition] << "' is not a valid TOR dump file.\n";
	std::cerr << "File '" << crashdumpFile << "' is not a valid TOR dump file.\n";
    return -6;
    }

    std::optional processorsSection = getProcessorsSection(*input);
    if (!processorsSection) {
     // std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
     std::cerr << "File '" << crashdumpFile<< "' does not contain 'PROCESSORS' or 'CPU' section\n";
      return -7;
    }
    // Create map with CPUs correlated with CPU sections
    auto cpuSections = getAllCpuSections(*processorsSection, cpus);
    // Check if valid CPU sections are available in input file
    if (cpuSections.size() == 0) {
     // std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
    std::cerr << "File '" << crashdumpFile<< "' does not contain any valid 'cpu' sections\n";
      return -8;
    }

    // Gather address_map if exists
    summary.memoryMap = getMemoryMap(cpuSections);

    IcxCpu cpu = IcxCpu();

    // Analyze TOR data in CPU sections
    IcxTOR tor = cpu.analyze(cpuSections);

    // Analyze MCA registers
    summary.mca = cpu.analyzeMca(cpuSections);

    // Analyze correctable PCIe AER registers
    summary.corAer = cpu.analyzeCorAer(cpuSections);

    // Analyze uncorrectable PCIe AER registers
    summary.uncAer = cpu.analyzeUncAer(cpuSections);

    // Get TSC registers data
    summary.tsc = cpu.getTscData(cpuSections);

    // Get Package Therm Status data
    auto thermStatus = cpu.getThermData(cpuSections);
    if (thermStatus)
    {
        summary.thermStatus = *thermStatus;
    }

    // Initialize report from collected data
    IcxReport report(summary, tor);

    // Generate JSON report from collected data
    json reportJson = report.createJSONReport();

    // Measure script time
    auto end = std::chrono::steady_clock::now();
    reportJson["_total_time"] = countDuration(end, start);

    // Dump report in 'pretty printed' JSON format
    std::cout << reportJson.dump(4) << std::endl;

	        // Dump the report to json file
        std::ofstream o("/var/crashdump/json/BAFI.json");
        o << std::setw(4) << reportJson.dump(4) << std::endl;
  }
  else if (summary.cpuType == "CPX") {

    // Gather CPUs from METADATA section
    std::vector<std::string> cpus = findCpus(*input);

    if (cpus.size() == 0) {
    //  std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
    std::cerr << "File '" << crashdumpFile<< "' is not a valid TOR dump file.\n";
      return -6;
    }

    std::optional processorsSection = getProcessorsSection(*input);
    if (!processorsSection) {
      //std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
      std::cerr << "File '" << crashdumpFile<< "' does not contain 'PROCESSORS' or 'CPU' section\n";
      return -7;
    }
    // Create map with CPUs correlated with CPU sections
    auto cpuSections = getAllCpuSections(*processorsSection, cpus);
    // Check if valid CPU sections are available in input file
    if (cpuSections.size() == 0) {
    //  std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
     std::cerr << "File '" << crashdumpFile<< "' does not contain any valid 'cpu' sections\n";
      return -8;
    }

    // Gather address_map if exists
    summary.memoryMap = getMemoryMap(cpuSections);

    CpxCpu cpu = CpxCpu();

    // Analyze TOR data in CPU sections
    CpxTOR tor = cpu.analyze(cpuSections);

    // Analyze MCA registers
    summary.mca = cpu.analyzeMca(cpuSections);

    // Analyze correctable PCIe AER registers
    summary.corAer = cpu.analyzeCorAer(cpuSections);

    // Analyze uncorrectable PCIe AER registers
    summary.uncAer = cpu.analyzeUncAer(cpuSections);

    // Get TSC registers data
    summary.tsc = cpu.getTscData(cpuSections);

    // Initialize report from collected data
    CpxReport report(summary, tor);

    // Generate JSON report from collected data
    json reportJson = report.createJSONReport();

    // Measure script time
    auto end = std::chrono::steady_clock::now();
    reportJson["_total_time"] = countDuration(end, start);

    // Dump report in 'pretty printed' JSON format
    std::cout << reportJson.dump(4) << std::endl;
  } else if (summary.cpuType == "SKX") {

    std::vector<std::string> cpus = findSocketsPurley(*input);
    if (cpus.size() == 0) {
   // std::cerr << "File '" << argv[crashdumpFileArgumentPosition]
std::cerr << "File '" << crashdumpFile<< "' is not a valid TOR dump file.\n";
    return -6;
    }

    SkxCpu cpu = SkxCpu();

    // Analyze TOR data in CPU sections
    SkxTOR tor = cpu.analyze(*input);

    // Analyze MCA registers
    summary.mca = cpu.analyzeMca(*input);

    // Analyze correctable PCIe AER registers
    summary.corAer = cpu.analyzeCorAer(*input);
    // Analyze uncorrectable PCIe AER registers
    summary.uncAer = cpu.analyzeUncAer(*input);

    // Initialize report from collected data
    SkxReport report(summary, tor);

    // Generate JSON report from collected data
    json reportJson = report.createJSONReport();

    // Measure script time
    auto end = std::chrono::steady_clock::now();
    reportJson["_total_time"] = countDuration(end, start);

    // Dump report in 'pretty printed' JSON format
    std::cout << reportJson.dump(4) << std::endl;
  }
  else {
    std::cerr << "Unrecognized CPU type. Supported CPUs: ICX, CPX, SKX.\n"
              << std::endl;
    return -9;
  }

  return 0;
}};
