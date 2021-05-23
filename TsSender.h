/*******************************************************************************
 * @file TsSender.h @brief Digital Devices Common Interface plugin for VDR.
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
#include <string>             /* std::string */
#include <unistd.h>           /* close() */
#include <vdr/thread.h>       /* cThread */
#include <vdr/ringbuffer.h>   /* cRingBufferLinear */

/*******************************************************************************
 * forward declarations.
 ******************************************************************************/
class cAdapter;



/*******************************************************************************
 * This class implements the physical interface to the CAM TS device.
 * It implements a send buffer and a sender thread to write the TS data
 * independent and with big chunks.
 ******************************************************************************/
class cTsSender: public cThread {
private:
  cAdapter& adapter;     //< the associated CI adapter
  int fd;                //< adapterX/secY fd write
  std::string devpath;   //< adapterX/secY device path
  cRingBufferLinear rb;  //< the send buffer
  cMutex mutex;          //< The synchronization mutex for rb write access
  int pkgCntR;           //< package read counter
  int pkgCntW;           //< package write counter
  int pkgCntRL;          //< package read counter last
  int pkgCntWL;          //< package write counter last

  bool clear;            //< true, when the buffer shall be cleared
  int cntSndDbg;         //< counter for data debugging
  volatile bool started;

  void CleanUp(void) { if (fd != -1) { close(fd); fd = -1; } }

  /* Before calling this function, it needs to be checked that at least
   * count bytes are free in the buffer.
   * count will be set to the real written data size.
   * Returns false, if not count bytes could be written.
   */
  bool PutAndCheck(const uint8_t* Data, int& Count);

public:
  /* Constructor, creates a new CAM TS send buffer.
   * @param Adapter - the CAM adapter this slot is associated
   * @param sec_fdw - read fd for the adapterX/secY
   * @param sec     - device path for adapterX/secY
   */
  cTsSender(cAdapter& Adapter, int sec_fdw, std::string& devNameCi);

  /* Destructor. */
  virtual ~cTsSender(void);

  bool Start(void);
  /* Waits for data in send buffer and sends them to the CAM. */
  virtual void Action(void);
  void Cancel(int waitSec = 0);

  void Clear(void) { clear = true; }

  std::string DevPath(void) { return devpath; }

  /* Write as most of the given data to the send buffer.
   * This function is thread save for multiple writers.
   * @param data the data to send
   * @param count the length of the data (have to be a multiple of TS_SIZE!)
   * @return the number of bytes actually written
   */
  int Write(const uint8_t* Data, int Count);

  /* Write *all* or *nothing* of the given data to the send buffer.
   * This function is thread save for multiple writers.
   * @param data the data to send
   * @param count the length of the data (have to be a multiple of TS_SIZE!)
   * @return true ... data copied
   *         false .. no data written to buffer
   */
  bool WriteAll(const uint8_t* Data, int Count);
};
