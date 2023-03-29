#include <algorithm>
#include <iostream>

#include "iconnect/ConnectionMgr.h"
#include "iconnect/CDebugFacade.h"
#include "iconnect/CSessionCtrl.h"
#include <chrono>

static uint64_t SystemTime_us()
{
  const auto tp = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
}

static void RawReadTest(isys::CDebugFacade & rDebugFacade)
{
  std::cout << "****************************************" << std::endl;
  std::cout << "Starting raw read test ..." << std::endl;
  
  const ADDRESS_64  c_aAddress    = 0x70000000;
  const ADDRESS_64  c_aNumMAUs    = 0x1;
  const uint32_t    c_dwNumReads  = 1000;

  uint64_t qwMinTime_us       = 0;
  uint64_t qwMaxTime_us       = 0;
  uint64_t qwTotalTime_us     = 0;
  uint64_t qwAvgTime_us       = 0;
  uint64_t qwTestStartTime_us = SystemTime_us();

  for (uint32_t I = 0; I < c_dwNumReads; I++)
  {
    std::vector<uint8_t> abyData;
    uint64_t qwStartTime_us = SystemTime_us();
    abyData = rDebugFacade.readMemory(isys::IConnectDebug::EAccessFlags::fMonitor, 0, c_aAddress, c_aNumMAUs, 1);
    uint64_t qwEndTime_us = SystemTime_us();

    uint64_t qwDiff_us = qwEndTime_us - qwStartTime_us;
    //std::cout << "Diff time [us]: " << std::dec << qwDiff_us << std::endl;

    // Increase total time
    qwTotalTime_us += qwDiff_us;

    // Set max time
    if (qwDiff_us > qwMaxTime_us)
    {
      qwMaxTime_us = qwDiff_us;
    }

    // Set min time
    if (qwMinTime_us == 0 || qwDiff_us < qwMinTime_us)
    {
      qwMinTime_us = qwDiff_us;
    }
  }

  qwAvgTime_us = qwTotalTime_us / c_dwNumReads;

  uint64_t qwTestEndTime_us = SystemTime_us();

  std::cout << "\nResults:" << std::endl;
  std::cout << "Number of reads: "    << std::dec << c_dwNumReads << std::endl;
  std::cout << "Min read time [us]: " << std::dec << qwMinTime_us << std::endl;
  std::cout << "Max read time [us]: " << std::dec << qwMaxTime_us << std::endl;
  std::cout << "Avg read time [us]: " << std::dec << qwAvgTime_us << std::endl;
  std::cout << "Raw read test is finished. Duration [ms]: " << std::dec << (qwTestEndTime_us - qwTestStartTime_us) / 1000 << std::endl;
  std::cout << "****************************************" << std::endl;
}

int main()
{
  try
  {
    // Connect to winIDEA (starting it if necessary)
    isys::ConnectionMgrSPtr connMgr = std::make_shared<isys::ConnectionMgr>();
    connMgr->connectMRU();

    isys::CSessionCtrl          sessionCtrl = isys::CSessionCtrl(connMgr);
    isys::CExecutionController  execCtrl    = isys::CExecutionController(connMgr);

    // Print the winIDEA version
    {
      isys::CWinIDEAVersion winIDEAVersion = connMgr->getWinIDEAVersion();
      std::cout << "Connected to winIDEA " << winIDEAVersion.toString() << std::endl;
    }

    // Reset CPU
    std::cout << "Stop..." << std::endl;
    isys::CDebugFacade debugFacade(connMgr);
    debugFacade.reset();

    // Disable polling of target status to remove large jitter during tests
    execCtrl.setPollingEnabled(false);

    // Execute the RawReadTest
    RawReadTest(debugFacade);

    // Enable polling of target status
    execCtrl.setPollingEnabled(true);

    // End the debug session
    std::cout << "Ending session..." << std::endl;
    sessionCtrl.end();

    std::cout << "Finished" << std::endl;
  }
  catch (const std::exception & ex)
  {
    std::cout << "Exception: " << ex.what() << std::endl;
  }

  return 0;
}