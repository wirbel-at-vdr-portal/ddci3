/*******************************************************************************
 * @file CamSlot.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include "CamSlot.h"
#include "CiAdapter.h"
#include "TsSender.h"
#include "Logging.h"

#include <vdr/remux.h>

extern bool IgnoreActiveFlag;   // global flag
extern bool ClearScramblingBit; // global flag

static const int SCT_DBG_TMO = 2000;   // 2000 milliseconds
static const int CNT_SCT_DBG_MAX = 20;


/*******************************************************************************
 * class cCiCamSlot
 ******************************************************************************/
cCiCamSlot::cCiCamSlot(cAdapter& Adapter, cTsSender& TsSend) :
   cCamSlot(&Adapter, true), adapter(Adapter), tsSend(TsSend),
   clear_rBuffer(false), delivered(false), active(false), cntSctPkt(0),
   cntSctPktL(0), cntSctClrPkt(0), cntSctDbg(0)
{
  log(3, std::string(__FUNCTION__) + ": " + Adapter.DevPath());

  MtdEnable();
}


cCiCamSlot::~cCiCamSlot(void) {
  _entering;

  StopIt();

  _leaving;
}


bool cCiCamSlot::Reset(void) {
  _entering;
  log(2, __FUNCTION__);

  bool ret = cCamSlot::Reset();
  if (ret)
     StopIt();

  _leaving;
  return ret;
}


void cCiCamSlot::StartDecrypting(void) {
  _entering;
  log(2, __FUNCTION__);

  mutex.Lock();    // to lock the processing against StopIt
  active = true;
  mutex.Unlock();  // need to unlock it before base class call to avoid deadlock

  if (!MtdActive())
     cCamSlot::StartDecrypting();

  _leaving;
}


void cCiCamSlot::StopDecrypting(void) {
  _entering;
  log(2, __FUNCTION__);

  cCamSlot::StopDecrypting();
  StopIt();

  _leaving;
}


uint8_t* cCiCamSlot::Decrypt(uint8_t* Data, int& Count) {
  /* Normally we would need to lock mutex. But because we only write the
   * packet into a send buffer or ignore them during the state change, it is
   * not worth to lock here.
   */

  if (Data)
     Count -= (Count % TS_SIZE);  // we write only whole TS frames

  if (!(active || IgnoreActiveFlag))
     return 0;

  /* WRITE */
  if (Data)
     Count = tsSend.Write(Data, Count);

  /* with MTD support active, decrypted TS packets are sent to the
   * individual MTD CAM slots in DataRecv(). */
  if (MtdActive())
     return 0;


  /* READ, Decrypt is called for each frame and we need to return the decoded
   * frame. But there is no "I_have_the_frame_consumed" function, so the
   * only chance we have is to delete now the last sent frame from the
   * buffer.*/
  if (delivered) {
     rBuffer.Del(TS_SIZE);
     delivered = false;
     }

  if (clear_rBuffer) {
     rBuffer.Clear();
     clear_rBuffer = false;
     }

  int cnt = 0;
  uint8_t* data = rBuffer.Get(cnt);

  if (!data || (cnt < TS_SIZE))
     data = 0;
  else {
     if (TsIsScrambled(data)) {
        ++cntSctPkt;
        if (ClearScramblingBit) {
           data[3] &= ~TS_SCRAMBLING_CONTROL;
           ++cntSctClrPkt;
           }
        }
     if ((cntSctPkt != cntSctPktL) && (cntSctDbg < CNT_SCT_DBG_MAX) && timSctDbg.TimedOut()) {
        cntSctPktL = cntSctPkt;
        ++cntSctDbg;
        log(3, "cCamSlot(" + tsSend.DevPath() + ") got " +
           std::to_string(cntSctPkt) + " scrambled packets from CAM");
        log(3, "cCamSlot(" + tsSend.DevPath() + ") clr " +
           std::to_string(cntSctClrPkt) + " scrambling control bits");
        timSctDbg.Set(SCT_DBG_TMO);
        }
     delivered = true;
     }

  return data;
}


bool cCiCamSlot::Inject(uint8_t* Data, int Count) {
  return tsSend.WriteAll(Data, Count);
}


int cCiCamSlot::DataRecv(uint8_t* Data, int Count) {
  if (!(active || IgnoreActiveFlag))
     return Count;   // not active, eat all the Data

  int written;

  if (MtdActive())
     written = MtdPutData(Data, Count);
  else {
     int free = rBuffer.Free();
     free -= free % TS_SIZE;   // write only whole packets
     if (free >= TS_SIZE) {
        if (free < Count)
           Count = free;
        written = rBuffer.Put(Data, Count);
        if (written != Count)
           log(1, std::string(__PRETTY_FUNCTION__) +
               ": Couldn't write previously checked free Data ?!? " +
               strerror(errno));
        }
     else
        written = 0;
     }

  return written;
}


void cCiCamSlot::StopIt(void) {
  cMutexLock MutexLock(&mutex);
  active = false;
  clear_rBuffer = true;
  delivered = false;
  cntSctPkt = 0;
  cntSctClrPkt = 0;
  cntSctDbg = 0;
  timSctDbg.Set(SCT_DBG_TMO);

  adapter.ClrBuffers();
}
