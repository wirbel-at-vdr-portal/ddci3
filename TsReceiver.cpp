/*******************************************************************************
 * @file TsReceiver.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include <vdr/tools.h>
#include "TsReceiver.h"
#include "Common.h"
#include "CiAdapter.h"
#include "Logging.h"


extern int SleepTimeout;
static const int CNT_REC_DBG_MAX = 100;


/*******************************************************************************
 * class cTsReceiver
 ******************************************************************************/
cTsReceiver::cTsReceiver(cAdapter& Adapter, int ci_fdr, std::string& sec) :
  cThread(), adapter(Adapter), fd(ci_fdr), devpath(sec),
  rb(BufferSize(), TS_SIZE, DebugBuffers, "CAM cTsReceiver"),
  pkgCntR(0), pkgCntW(0), pkgCntRL(0), pkgCntWL(0), clear(false), retry(0),
  cntRecDbg(0), tsdeliver(*this, sec)
{
  // don't use adapter in this function, unless you know what you are doing!

  SetDescription("cTsReceiver %s", devpath.c_str());
  log(3, "cTsReceiver " + devpath);
}


cTsReceiver::~cTsReceiver(void) {
  _entering;

  Cancel(3);
  CleanUp();

  _leaving;
}


bool cTsReceiver::Start(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "            " + devpath);

  return cThread::Start();
}


void cTsReceiver::Cancel(int waitSec) {
  _entering;

  cThread::Cancel(waitSec);

  _leaving;
}


void cTsReceiver::Deliver(void) {
  while (Running()) {
     if (clear) {
        rb.Clear();
        pkgCntW = 0;
        pkgCntR = 0;
        clear = false;
        retry = 0;
        cntRecDbg = 0;
        }

     int cnt = 0;
     uint8_t* data = rb.Get(cnt);
     if (!data || cnt < TS_SIZE)
        continue;

     int skipped;
     uint8_t* frame = CheckTsSync(data, cnt, skipped);
     if (skipped) {
        log(1, std::string(__PRETTY_FUNCTION__) +
            ": skipped " + std::to_string(skipped) +
            " bytes to sync on start of TS packet - " + strerror(errno));
        rb.Del(skipped);
        cnt -= skipped;
        }

    if (cnt < TS_SIZE)
        continue;

    /* write only whole packets, because CheckTsSync will
     * fail in next round otherwise
     */
    cnt -= cnt % TS_SIZE;

    int written = adapter.DataRecv( frame, cnt );
    if (written != 0) {
       rb.Del( written );
       retry = 0;
       pkgCntR += written / TS_SIZE;
       }
    else {
       if (retry++ < 3) {
          /* The receive buffer of the adapter is full,
           * so we need to wait a little bit. */
          cCondWait::SleepMs(SleepTimeout);
          }
       else {
          log(1, "Can't write packet VDR CamSlot for CI adapter " +
              std::string(adapter.DevPath()) + ")");
          rb.Del( TS_SIZE );
          retry = 0;
          }
       }
    }
}


void cTsReceiver::Action(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "   " + adapter.DevPath());

  cPoller Poller(fd);

  if (!tsdeliver.Start()) {
     log(1, std::string(__PRETTY_FUNCTION__) +
         ": Couldn't start deliver thread - " + strerror(errno));
     return;
     }

  rb.SetTimeouts(0, SleepTimeout);
  cTimeMs t(DBG_PKG_TMO);

  while(Running()) {
    if (Poller.Poll(SleepTimeout)) {
       errno = 0;
       int r = rb.Read(fd);
       if ((r < 0) && FATALERRNO) {
          if (errno == EOVERFLOW)
             log(1, std::string(__PRETTY_FUNCTION__) +
                 ": Driver buffer overflow on file " + devpath +
                 ":" + strerror(errno));
          else {
             log(1, std::string(__PRETTY_FUNCTION__) +
                 ": fatal error on file " + devpath + ":" + strerror(errno));
             break;
             }
          }
       if (r > 0) {
          if (cntRecDbg < CNT_REC_DBG_MAX) {
             ++cntRecDbg;
             log(4, "cTsReceiver for " + devpath + " received data from CAM ###");
             }
          pkgCntW += r / TS_SIZE;
          }
       }

    if (t.TimedOut()) {
       if ((pkgCntR != pkgCntRL) || (pkgCntW != pkgCntWL)) {
          log(4, "cTsReceiver for " + devpath +
              " CAM buff wr(CAM ->):" + std::to_string(pkgCntW) +
              ", rd:" + std::to_string(pkgCntR));
          pkgCntRL = pkgCntR;
          pkgCntWL = pkgCntW;
          }
       t.Set(DBG_PKG_TMO);
       }
    } // while(Running())

  tsdeliver.Cancel(3);
  CleanUp();

  _leaving;
}
