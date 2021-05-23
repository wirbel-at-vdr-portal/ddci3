/*******************************************************************************
 * @file CamSlot.h @brief Digital Devices Common Interface plugin for VDR.
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
#include <vdr/ci.h>
#include "Common.h"

/*******************************************************************************
 * forward declarations.
 ******************************************************************************/
class cAdapter;
class cTsSender;
extern bool DebugBuffers;


/*******************************************************************************
 * This class implements the logical interface to one slot of the CAM device.
 ******************************************************************************/
class cCiCamSlot: public cCamSlot {
private:
  //-------------------------
  class cReceiveBuffer: public cRingBufferLinear {
     public:
        cReceiveBuffer() : cRingBufferLinear(BufferSize(), TS_SIZE, DebugBuffers,
                           "DDCI Slot cReceiveBuffer" ) {}
        virtual ~cReceiveBuffer() {}
    };
  //-------------------------
  cAdapter& adapter;       //< the adapter of this CAM slot
  cMutex mutex;            //< the synchronization mutex for Start/StopDecrypting
  cTsSender& tsSend;       //< the CAM TS sender
  cReceiveBuffer rBuffer;  //< the receive buffer
  bool clear_rBuffer;      //< true, when the receive buffer shall be cleared
  bool delivered;          //< true, if Decrypt did deliver data at last call
  bool active;             //< true, if this slot does decrypting
  int cntSctPkt;           //< number of scrambled packets got from CAM
  int cntSctPktL;          //< number of scrambled packets got from CAM last
  int cntSctClrPkt;        //< number of cleared scrambling control bits
  int cntSctDbg;           //< counter for scrambling control debugging
  cTimeMs timSctDbg;       //< timer for scrambling control debugging

  void StopIt(void);

public:
  /* Constructor, creates a new CAM slot for the given adapter.
   * The adapter will take care of deleting the CAM slot, so the
   * caller must not delete it!
   * @param Adapter - the associated CAM adapter
   * @param TsSend  - the buffer for the TS packets
   */
  cCiCamSlot(cAdapter& Adapter, cTsSender& TsSend);

  /* Destructor. */
  virtual ~cCiCamSlot(void);

  /* see <vdr/ci.h> for the following functions */

  virtual bool Reset(void);
  virtual void StartDecrypting(void);
  virtual void StopDecrypting(void);

  /* Copy the given TS packet(s) to the CAM TS send buffer and
   * return the next decrypted TS packet from the CAM TS receive buffer.
   *
   * @param Data the TS packet(s) to decrypt
   * @param Count the number of bytes in Data (n * TS_SIZE).
   *        On function return it is set to the number of bytes
   *        consumed from Data. 0 in case the CAM send buffer is full.
   * @return A pointer to the first TS packet in the CAM receive buffer, or
   *        0, if the CAM receive buffer buffer is empty.
   */
  virtual uint8_t* Decrypt(uint8_t* Data, int& Count);

  /* This function will copy the given TS packet to the CAM TS send
   * buffer and return true if this was possible or false. In the latter case
   * nothing is copied at all.
   * @param Data the TS packet(s) to sent to the CAM; it have to point to the
   *        beginning of a packet (start with TS_SYNC_BYTE).
   * @param count the number of bytes in data (shall be at least TS_SIZE);
   *        have to be a multiple of TS_SIZE.
   * @return true ... Data copied
   *         false .. Nothing written to send buffer
   */
  virtual bool Inject(uint8_t* Data, int Count);

  /* Deliver the received CAM TS Data to the CAM slot.
   * @param data the received TS packet(s) from the CAM; it have to point to
   *        the beginning of a packet (start with TS_SYNC_BYTE).
   * @param count the number of bytes in data (shall be at least TS_SIZE).
   * @return the number of bytes actually processed
   */
  int DataRecv(uint8_t* Data, int Count);

  void StartMtd(void) { MtdEnable(); }
};
