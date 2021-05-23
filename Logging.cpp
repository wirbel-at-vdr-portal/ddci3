/*******************************************************************************
 * @file Logging.cpp @brief Digital Devices Common Interface plugin for VDR.
 * Copyright (c) 2013 - 2014 by Jasmin Jessich.  All Rights Reserved.
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
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <ctime>
#include <vdr/tools.h>
#include "Logging.h"

extern bool LogToSyslog;   // config logging dest variable

/*******************************************************************************
 * class cLogger
 ******************************************************************************/
class cLogger {
private:
  std::ofstream ofs;
  std::mutex mutex;
public:
  cLogger(const char* filename) { ofs.open(filename); }
  ~cLogger()                    { ofs.close(); }

  void Put(int level, std::string& msg) {
     auto now  = std::chrono::system_clock::now();
     auto time = std::chrono::system_clock::to_time_t(now);

     const std::lock_guard<std::mutex> lock(mutex);

     std::string Now(std::ctime(&time));
     Now.pop_back();

     ofs << Now << " : ";

     if (level == 1)      ofs << "[ERROR]: ****** ";
     else if (level == 2) ofs << "[INFO ]: ";
     else                 ofs << "[DEBUG]: ";

     ofs << msg << std::endl;
     ofs.flush();
     }
};


// an instance of cLogger:
cLogger Logger("/var/log/ddci3.log");



void log(int level, std::string msg) {
  if (LogLevel >= level) {
     if (LogToSyslog) {
        if (level == 1)      syslog_with_tid(LOG_ERR, msg.c_str());
        else if (level == 2) syslog_with_tid(LOG_INFO, msg.c_str());
        else                 syslog_with_tid(LOG_DEBUG, msg.c_str());
        }
     else
        Logger.Put(level, msg);
     }
}

void logEntering(std::string fname) {
  fname.insert(0, "entering ");
  Logger.Put(3, fname);
}

void logLeaving(std::string fname) {
  fname.insert(0, "leaving ");
  Logger.Put(3, fname);
}
