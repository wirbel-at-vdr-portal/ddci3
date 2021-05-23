/*******************************************************************************
 * @file TsSender.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include <vdr/tools.h>
#include "TsSender.h"
#include "Common.h"
#include "CiAdapter.h"
#include "Logging.h"

extern int SleepTimeout;
static const int CNT_SND_DBG_MAX = 100;


/*******************************************************************************
 * class cTsSender
 ******************************************************************************/

cTsSender::cTsSender(cAdapter& Adapter, int sec_fdw, std::string& sec) :
   cThread(), adapter(Adapter), fd(sec_fdw), devpath(sec),
   rb(BufferSize(), TS_SIZE, DebugBuffers, "CAM cTsSender"),
   pkgCntR(0), pkgCntW(0), pkgCntRL(0), pkgCntWL(0), clear(false), cntSndDbg(0),
   started(false)
{
  // don't use adapter in this function, unless you know what you are doing!

  SetDescription("cTsSender %s", devpath.c_str());
  log(3, std::string(__FUNCTION__) + "   " + devpath);
}


cTsSender::~cTsSender(void) {
  _entering;

  Cancel(3);
  CleanUp();

  _leaving;
}


bool cTsSender::Start(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "              " + devpath);

  if (started) {
     log(1, std::string(__PRETTY_FUNCTION__) + "              " + devpath + " started twice!!");
     return false;
     }
  started = true;
  return cThread::Start();
}


void cTsSender::Cancel(int waitSec) {
  _entering;

  cThread::Cancel(waitSec);

  _leaving;
}


int cTsSender::Write(const uint8_t* Data, int Count) {
  cMutexLock MutexLockW(&mutex);

  int free = rb.Free();
  if (free > Count)
     free = Count;
  free -= free % TS_SIZE;  // only whole TS frames must be written
  if (free > 0)
     PutAndCheck(Data, free);

  return free;
}


bool cTsSender::WriteAll(const uint8_t* Data, int Count) {
  cMutexLock MutexLockW(&mutex);

  if (Count % TS_SIZE)    // have to be a multiple of TS_SIZE
     return false;

  if (rb.Free() < Count)  // all the packets need to be written at once
     return false;

  return PutAndCheck(Data, Count);
}



bool cTsSender::PutAndCheck(const uint8_t* Data, int& Count) {
  bool ret = true;

  int written = rb.Put(Data, Count);
  pkgCntW += written / TS_SIZE;
  if (written != Count) {
     log(1, std::string(__PRETTY_FUNCTION__) +
         ": Couldn't write previously checked free data ?!? - " +
         strerror(errno));
     ret = false;
     }

  Count = written;
  return ret;
}



void cTsSender::Action(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "     " + adapter.DevPath());

  const int run_check_tmo = SleepTimeout;

  rb.SetTimeouts(0, run_check_tmo);
  cTimeMs t(DBG_PKG_TMO);

  while(Running()) {
     if (clear) {
        rb.Clear();
        pkgCntW = 0;
        pkgCntR = 0;
        clear = false;
        cntSndDbg = 0;
        }

     int cnt = 0;
     uint8_t* data = rb.Get(cnt);
     if (data && cnt >= TS_SIZE) {
        int skipped;
        uint8_t* frame = CheckTsSync(data, cnt, skipped);
        if (skipped) {
           log(1, "skipped " + std::to_string(skipped) +
               " bytes to sync on start of TS packet: " + strerror(errno));
           rb.Del(skipped);
           }

        int len = cnt - skipped;
        len -= (len % TS_SIZE);     // only whole TS frames must be written
        if (len >= TS_SIZE) {
           int w = WriteAllOrNothing(fd, frame, len, 5 * run_check_tmo, run_check_tmo);
           if (w >= 0) {
              int remain = len - w;
              if (remain > 0) {
                 log(1, "couldn't write all data to CAM " + devpath +
                     ": " + strerror(errno));
                 len -= remain;
                 }
              if (cntSndDbg < CNT_SND_DBG_MAX) {
                 ++cntSndDbg;
                 log(4, "cTsSender for " + devpath + " wrote data to CAM ###");
                 }
              }
           else {
              log(1, "couldn't write to CAM " + devpath + ":" + strerror(errno));
              break;
              }
           rb.Del(w);
           pkgCntR += w / TS_SIZE;
           }
        }

     if (t.TimedOut()) {
        if ((pkgCntR != pkgCntRL) || (pkgCntW != pkgCntWL)) {
           log(4, "cTsSender for " + devpath +
               " CAM buff rd(-> CAM):" + std::to_string(pkgCntR) +
               ", wr:" + std::to_string(pkgCntW));
           pkgCntRL = pkgCntR;
           pkgCntWL = pkgCntW;
           }
        t.Set(DBG_PKG_TMO);
        }
     } // while(Running())

  CleanUp();
  _leaving;
}
