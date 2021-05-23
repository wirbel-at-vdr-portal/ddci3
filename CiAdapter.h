/*******************************************************************************
 * @file CiAdapter.h @brief Digital Devices Common Interface plugin for VDR.
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
#include <vdr/ci.h>
#include "TsSender.h"
#include "TsReceiver.h"



/*******************************************************************************
 * forward declarations.
 ******************************************************************************/
class cCiCamSlot;



/*******************************************************************************
 * a DD CI device node with names and file descriptors
 ******************************************************************************/
class caDevice {
public:
  std::string sec;  /* /dev/dvb/adapterX/secY          */
  std::string ca;   /* /dev/dvb/adapterX/caY           */
  int Number;       /* Y, see above.                   */
  int fd;           /* rw    file handle adapterX/caY  */
  int sec_fdw;      /* write file handle adapterX/secY */
  int sec_fdr;      /* read  file handle adapterX/secY */
public:
  caDevice(void) : Number(-1), fd(-1), sec_fdw(-1), sec_fdr(-1) {}
};



/*******************************************************************************
 * This class implements the physical interface to the CAM device.
 ******************************************************************************/
class cAdapter: public cCiAdapter {
private:
  int fd;               //< adapterX/caY device file handle
  std::string devpath;  //< adapterX/caY device path
  cTsSender   ciSend;   //< the CAM TS sender   adapterX/secY
  cTsReceiver ciRecv;   //< the CAM TS receiver adapterX/secY
  volatile bool started;
  cTimeMs StatusTimer;
  eModuleStatus status;
  int reboots;
  cTimeMs StartTimer;

  // FIXME: after VDR base class change, this is not necessary
  cCiCamSlot* CamSlot;  //< the one and only slot of a DD CI adapter

  void CleanUp(void) { if (fd != -1) { close(fd); fd = -1; } }
  eModuleStatus GetModuleStatus(int Slot);

protected:
  /* see file ci.h in the VDR include directory for the description of
   * the following functions */

  virtual void Action(void);
  virtual int  Read(uint8_t* Buffer, int MaxLength);
  virtual void Write(const uint8_t* Buffer, int Length);
  virtual bool Reset(int Slot);
  virtual eModuleStatus ModuleStatus(int Slot);
  virtual bool Assign(cDevice* Device, bool Query = false);

public:
  /* Constructor, checks for the available slots of the CAM and starts the
   * controlling thread.
   * @param ca_fd  the rw    file handle for the adapterX/caY device
   * @param sec_fdw - the write file handle for the adapterX/secY device
   * @param sec_fdr - the read  file handle for the adapterX/secY device
   * @param ca      - device path for adapterX/caY
   * @param sec     - device path for adapterX/secY
   */
  cAdapter(int ca_fd, int sec_fdw, int sec_fdr, std::string& ca, std::string& sec);
  cAdapter(caDevice& Ca);

  /* Destructor */
  virtual ~cAdapter(void);

  /* Deliver the received CAM TS Data to the receive buffer.
   * @param data the received TS packet(s) from the CAM; it have to point to
   *        the beginning of a packet (start with TS_SYNC_BYTE).
   * @param count the number of bytes in data (shall be at least TS_SIZE).
   * @return the number of bytes actually processed
   */
  int DataRecv(uint8_t* Data, int Count);

  /* get the caX device name */
  std::string DevPath(void) { return devpath; }

  /* clear the CAM send and receive buffer */
  void ClrBuffers(void);

  /* stop this thread */
  void Cancel(int waitSec = 0);
};
