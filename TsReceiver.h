/*******************************************************************************
 * @file TsReceiver.h @brief Digital Devices Common Interface plugin for VDR.
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
#pragma once
#include <string>
#include <vdr/thread.h>
#include <vdr/remux.h>   // TS_SIZE, TS_SYNC_BYTE
#include <vdr/ringbuffer.h>
#include "TsDeliver.h"

/*******************************************************************************
 * forward declarations.
 ******************************************************************************/
class cAdapter;


/*******************************************************************************
 * This class implements the physical interface to the CAM TS device.
 * It implements a receive buffer and a receiver thread the TS data
 * independent and with big junks.
 ******************************************************************************/
class cTsReceiver : public cThread {
private:
  cAdapter& adapter;     //< the associated CI adapter
  int fd;                //< adapterX/secY device read file handle
  std::string devpath;   //< adapterX/secY device path
  cRingBufferLinear rb;  //< the CAM read buffer
  int pkgCntR;           //< packages read from buffer
  int pkgCntW;           //< packages written to buffer
  int pkgCntRL;          //< package read counter last
  int pkgCntWL;          //< package write counter last
  bool clear;            //< true, when the buffer shall be cleared
  int retry;             //< number of retries to send a packet
  int cntRecDbg;         //< counter for data debugging
  cDeliver tsdeliver;    //< TS Data deliver thread
  volatile bool started;

  void CleanUp(void) { if (fd != -1) { close(fd); fd = -1; } }

public:
  /* Constructor, creates a new CAM TS receiver object.
   * @param Adapter - the associated CAM adapter
   * @param ci_fdr  - open file handle for adapterX/secY
   * @param sec     - device path for adapterX/secY
   */
  cTsReceiver(cAdapter& Adapter, int ci_fdr, std::string& sec);

  /* Destructor. */
  virtual ~cTsReceiver(void);

  bool Start(void);
  /* Waits for data present from the CAM and tries to deliver it. */
  virtual void Action(void);
  void Cancel(int waitSec = 0);

  void Clear(void) { clear = true; }
  void Deliver(void);
};
