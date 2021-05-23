/*******************************************************************************
 * @file ddci3.cpp @brief Digital Devices Common Interface plugin for VDR.
 * Copyright (c) 2013 - 2017 by Jasmin Jessich.  All Rights Reserved.
 * Copyright (c) 2021 by Winfried Köhler.  All Rights Reserved.
 * Contributor(s):
 * License: GPLv2
 *
 * This file is part of vdr-plugin-ddci3.
 *
 * vdr-plugin-ddci3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * vdr-plugin-ddci3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-ddci3.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <getopt.h>
#include <vdr/plugin.h>
#include <sys/ioctl.h>
#include <linux/dvb/ca.h>
#include "CiAdapter.h"
#include "Logging.h"
#include "FileList.h"

static const char *VERSION = "2021.01.23_15h27";
static const char *DESCRIPTION = "Digital Devices CI-Adapter";

int  LogLevel           = 2;      // 1 = error, 2 = info, 3 = debug, 4 = debug + debugBuffers
bool LogToSyslog        = true;   // true: log to syslog, false: log to /var/log/ddci3.log
bool IgnoreActiveFlag   = false;  // true: active flag in cCiCamSlot is ignored
int  BufSize            = 1500;   // in multiple of 188 bytes, 1500..10000
bool DebugBuffers       = false;  // debug RingBuffer sizes
bool ClearScramblingBit = false;  // clear the scambling control bit before packet is send to VDR
int  SleepTimeout       = 100;    // CAM receive/send/deliver thread sleep timer in ms, 100..1000




/*******************************************************************************
 * The plugin based CI/CAM device.
 ******************************************************************************/
class cPluginDDCI3 : public cPlugin {
private:
  std::vector<cAdapter*> adapters;
  std::vector<caDevice> caDevices;
  bool Find(void);

public:
  cPluginDDCI3(void)                                { adapters.reserve(MAXDEVICES); }
  virtual ~cPluginDDCI3(void)                       { for(auto a:adapters) delete a; }
  virtual const char* Version(void)                 { return tr(VERSION); }
  virtual const char* Description(void)             { return tr(DESCRIPTION); }
  virtual const char* CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char* argv[]);

  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void)   { for(auto a:adapters) a->Cancel(3); }
};



/*******************************************************************************
 * Find CI's and open them in cPluginDDCI3::Initialize().
 * Give the kernel driver time to load firmware until we startup them
 * later in cPluginDDCI3::Start()
 ******************************************************************************/
bool cPluginDDCI3::Find(void) {
  _entering;

  /*----------------------------------------------------------------*/
  auto OpenDevice = [](std::string name, int mode) -> int {    
     int fd = open(name.c_str(), mode );
     if (fd < 0) {
        std::string Mode;
        if (mode & O_RDONLY)   Mode += "READ";
        if (mode & O_WRONLY)   Mode += "WRITE";
        if (mode & O_NONBLOCK) Mode += "|O_NONBLOCK";
        log(1, "Couldn't open " + name + " (" + Mode + "): " + strerror(errno));
        }
     return fd;
     };
  /*----------------------------------------------------------------*/

  for(auto adapter:cFileList("/dev/dvb", "adapter").List()) {
     adapter.insert(0, "/dev/dvb/");

     if (cFileList(adapter, "frontend").List().size())
        continue; /* we found frontend<N> -> not a DD CI */

     auto devices = std::move(cFileList(adapter, "ci").List());
     auto secdevs = std::move(cFileList(adapter, "sec").List());
     devices.insert(devices.end(), secdevs.begin(), secdevs.end());

     for(auto dev:devices) {
        log(2, "found " + adapter + '/' + dev);
        caDevice caDev;

        if (dev.find("ci") == 0)
           caDev.Number = std::stoi(dev.c_str() + 2);
        else if (dev.find("sec") == 0)
           caDev.Number = std::stoi(dev.c_str() + 3);

        caDev.ca  = adapter + '/' + "ca" + std::to_string(caDev.Number);
        caDev.sec = adapter + '/' + dev;
        caDev.fd      = OpenDevice(caDev.ca, O_RDWR);
        caDev.sec_fdw = OpenDevice(caDev.sec, O_WRONLY);
        caDev.sec_fdr = OpenDevice(caDev.sec, O_RDONLY | O_NONBLOCK);


        if ((caDev.fd > 0) and (caDev.sec_fdw > 0) and (caDev.sec_fdr) > 0)
           caDevices.push_back(caDev);
        else {
           if (caDev.fd      > 0) close(caDev.fd);
           if (caDev.sec_fdw > 0) close(caDev.sec_fdw);
           if (caDev.sec_fdr > 0) close(caDev.sec_fdr);
           }
        }
     }

  _leaving;
  return caDevices.size() > 0;
}


bool cPluginDDCI3::Initialize(void) {
  Find();
  return true;
}


bool cPluginDDCI3::Start(void) {
  log(3, "=== entering " + std::string(__PRETTY_FUNCTION__) + " ===");

  log(2, std::string(PLUGIN_NAME_I18N) + " - " + std::string(VERSION) +
      " (compiled for VDR " + std::string(VDRVERSION) + ")");

  if (BufSize != 1500)      log(2, "Buffer size " + std::to_string(BufSize) + " packets");
  if (SleepTimeout != 100)  log(2, "SleepTimeout " + std::to_string(SleepTimeout) + "ms");
  if (IgnoreActiveFlag)     log(2, "Ignore-active-flag activated");
  if (ClearScramblingBit)   log(2, "Clear scrambling control bit activated");
  if (DebugBuffers)         log(2, "debug RingBuffer sizes");


  std::sort(caDevices.begin(), caDevices.end(),
      [](caDevice a, caDevice b) -> bool { return a.sec.compare(b.sec); });

  for(auto d:caDevices) {
     log(2, "-- new CI Adapter " + d.ca + " --");
     adapters.push_back(new cAdapter(d));
     log(2, "------------------------------------------");
     cCondWait::SleepMs(2500);
     }

  caDevices.clear();

  log(3, "=== leaving  " + std::string(__PRETTY_FUNCTION__) + " ===");
  return true;
}


bool cPluginDDCI3::ProcessArgs(int argc, char* argv[]) {
  static struct option long_options[] = {
     { "ignact"       , no_argument      , NULL, 'A' },
     { "bufsz"        , required_argument, NULL, 'b' },
     { "clrsct"       , no_argument      , NULL, 'c' },
     { "loglevel"     , required_argument, NULL, 'l' },
     { "local"        , required_argument, NULL, 'L' },
     { "sleeptimer"   , required_argument, NULL, 't' },
     { "debug-buffers", no_argument      , NULL, 129 },
     { NULL           , no_argument      , NULL,  0  }};

  int c;
  while((c = getopt_long(argc, argv, "Ab:cl:Lt:", long_options, 0)) > 0) {
     switch(c) {
        case 'A':
           IgnoreActiveFlag = true;
           break;
        case 'b':
           if ((sscanf(optarg, "%d", &BufSize) < 1) or
                 (BufSize < 1500) or (BufSize > 10000)) {
              std::cerr << "Invalid buffer size" << std::endl;
              return false;
              }
           break;
        case 'c':
           ClearScramblingBit = true;
           break;
        case 'l':
           if ((sscanf(optarg, "%d", &LogLevel) < 1) or (LogLevel > 4)) {
              std::cerr << "Invalid log level entered" << std::endl;
              return false;
              }
           break;
        case 'L':
           LogToSyslog = false;
           break;
        case 't':
           if ((sscanf( optarg, "%d", &SleepTimeout) < 1) or 
                 (SleepTimeout > 1000)) {
              std::cerr << "Invalid sleep timer value entered" << std::endl;
              return false;
              }
           break;
        case 129:
           DebugBuffers = true;
           break;
        default:
           std::cerr << "Unknown option found" << std::endl;
           return false;
        }
     }
  return true;
}


const char* cPluginDDCI3::CommandLineHelp(void) {
  static const char* help =
     "  -A, --ignact        ignore active flag; speeds up channel switching to\n"
     "                      decryted channels\n"
     "  -b, --bufsz         CAM receive/send buffer size in packets a 188 bytes\n"
     "                      default: 1500, max: 10000\n"
     "  -c, --clrsct        clear the scambling control bit before the\n"
     "                      packet is send to VDR\n"
     "      --debug-buffers debug RingBuffer sizes\n"      
     "  -l, --loglevel      0/1/2/3 log nothing/error/info/debug\n"
     "  -L, --local         log to /var/log/ddci3.log instead of syslog\n"
     "  -t, --sleeptimer    CAM receive/send/deliver thread sleep timer in ms\n"
     "                      default: 100, max: 1000\n"
     ;

  return help;
}

VDRPLUGINCREATOR(cPluginDDCI3); // Don't touch this!
