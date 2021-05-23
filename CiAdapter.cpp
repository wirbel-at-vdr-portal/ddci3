/*******************************************************************************
 * @file CiAdapter.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include <sys/ioctl.h>
#include <linux/dvb/ca.h>
#include <vdr/device.h>
#include <assert.h>
#include "CiAdapter.h"
#include "CamSlot.h"
#include "Logging.h"


/*******************************************************************************
 * !!! NOTE: Most of the code is copied from <vdr/dvbci.c>
 ******************************************************************************/




/*******************************************************************************
 * class cAdapter
 ******************************************************************************/
cAdapter::cAdapter(caDevice& Ca) : cAdapter(Ca.fd, Ca.sec_fdw, Ca.sec_fdr, Ca.ca, Ca.sec) {}

cAdapter::cAdapter(int ca_fd, int sec_fdw, int sec_fdr, std::string& ca, std::string& sec) :
  fd(ca_fd),
  devpath(ca),
  ciSend(*this, sec_fdw, devpath),
  ciRecv(*this, sec_fdr, devpath),
  started(false), reboots(0),
  CamSlot(nullptr)
{
  log(3, std::string(__FUNCTION__) + "    " + devpath);
  ioctl(fd, CA_RESET);
  StartTimer.Set(20000);
  SetDescription("cAdapter %s", devpath.c_str());
  ca_caps_t Caps;
  if (ioctl(fd, CA_GET_CAP, &Caps) == 0) {
     if ((Caps.slot_type & CA_CI_LINK) != 0) {
        int NumSlots = Caps.slot_num;
        if (NumSlots > 0) {
           for(int i = 0; i < NumSlots; i++) {
              if (!CamSlot)
                 CamSlot = new cCiCamSlot(*this, ciSend);
              else
                 log(1, "CAM(" + devpath + ") Currently only ONE CAM slot supported");
              }

           std::string SlotType;
           if (Caps.slot_type & CA_CI)      SlotType += "CI high level interface,";
           if (Caps.slot_type & CA_CI_LINK) SlotType += "CI link layer level interface,";
           if (Caps.slot_type & CA_CI_PHYS) SlotType += "CI physical layer level interface,";
           if (Caps.slot_type & CA_DESCR)   SlotType += "built-in descrambler,";
           if (Caps.slot_type & CA_SC)      SlotType += "simple smart card interface,";
           if (SlotType.size()) SlotType.pop_back();

           log(3, "cAdapter(" + devpath + ") created: " +
               std::to_string(Caps.slot_num) + " Slots, " + 
               SlotType);

           status = msNone;
           StatusTimer.Set(1500);
           Start();
           }
        else
           log(1, devpath + "no CAM slots");
        }
     else
        log(2, devpath + ": no CI link layer interface");
     }
  else
     log(1, "CA_GET_CAP failed for CAM " + devpath);

  ciSend.Start();
  ciRecv.Start();
}


cAdapter::~cAdapter(void) {
  _entering;

  Cancel(3);
  CleanUp();

  _leaving;
}


int cAdapter::DataRecv(uint8_t* Data, int Count) {
  if (!CamSlot)
     return Count; // no slot, eat all the data

  return CamSlot->DataRecv(Data, Count);
}


void cAdapter::ClrBuffers(void) {
  ciSend.Clear();
  ciRecv.Clear();
}


void cAdapter::Cancel(int waitSec) {
  cThread::Cancel(waitSec);
}


void cAdapter::Action(void) {
  if (started) {
     log(1, std::string(__PRETTY_FUNCTION__) + "      " + devpath + " started twice!!");
     return;
     }
  started = true;

  log(3, std::string(__PRETTY_FUNCTION__) + "      " + devpath);
  cCiAdapter::Action();

  /* thread stopped */
  ciSend.Cancel(3);
  ciRecv.Cancel(3);

  _leaving;
}


int cAdapter::Read(uint8_t* Buffer, int MaxLength) {
  if (Buffer && MaxLength > 0) {
     struct pollfd pfd[1];

     pfd[0].fd = fd;
     pfd[0].events = POLLIN;
     if (poll(pfd, 1, CAM_READ_TIMEOUT) > 0 && (pfd[0].revents & POLLIN)) {
        int n = safe_read(fd, Buffer, MaxLength);
        if (n >= 0)
           return n;
        log(1, "can't read from CI adapter (" + devpath + ") : " + strerror(errno));
        }
     }
  return 0;
}

void cAdapter::Write(const uint8_t* Buffer, int Length) {
  if (Buffer && Length > 0) {
     if (safe_write(fd, Buffer, Length) != Length) {
        log(1, "can't write to " + devpath + ", Length = " + std::to_string(Length) + ": " + strerror(errno));
        if (errno == EAGAIN)
           log(1, "hmm - was EAGAIN not catched by safe_write?");
        else if (!StartTimer.TimedOut() && ((errno == EIO) or (errno == EINVAL))) {
           assert((errno == 0));

         //assert(reboots++ < 3);
         //if (errno == EINVAL)
         //   log(1, "looks like Length > ca->slot_info[slot].link_buf_size");
         //else
         //   log(1, "fatal I/O error on the CAM slot. Resetting it.");
         //CamSlot->CancelActivation();
         //ioctl(fd, CA_RESET);
         //status = msNone;
         //StatusTimer.Set(2000);
         //CamSlot->Assign(0);
         //for(int i = 0; i < cDevice::NumDevices(); i++)
         //   CamSlot->Assign(cDevice::GetDevice(i));
         //CamSlot->StartMtd();
         //
         //// try to recover CAM by channel switch now..
         //const cChannel* Channel = nullptr;
         //if (*Setup.InitialChannel) {
         //   LOCK_CHANNELS_READ;
         //   Channel = Channels->GetByChannelID(tChannelID::FromString(Setup.InitialChannel));
         //   }
         //if (Channel && cDevice::GetDeviceForTransponder(Channel,1)) {
         //   bool result = (!cDevice::GetDeviceForTransponder(Channel,1)->SwitchChannel(Channel, true));
         //   if (result)
         //      log(2, "switching to channel " + std::string(Channel->Name()));
         //   else
         //      log(2, "switching to channel " + std::string(Channel->Name()) + " failed.");
         //   }
           }
        }
     }
}


bool cAdapter::Reset(int Slot) {
  ClrBuffers();
  if (ioctl(fd, CA_RESET) == 0) {
  //if (ioctl(fd, CA_RESET, 1 << Slot) == 0)  {
     log(3, std::string(__FUNCTION__) + "       " + devpath + " - " + std::to_string(Slot));
     return true;
     }
  else {
     log(1, std::string(__FUNCTION__) + "       " + devpath + " - " + std::to_string(Slot) +
         " failed: " + strerror(errno));
     return false;
     }
}


eModuleStatus cAdapter::ModuleStatus(int Slot) {
  // vdr-2.4.6 aggressivly calls ModuleStatus() - protect CAM from beeing polled to often.
  if (StatusTimer.TimedOut()) {
     StatusTimer.Set(1000);
     status = GetModuleStatus(Slot);
     }
  return status;
}


eModuleStatus cAdapter::GetModuleStatus(int Slot) {
  log(3, std::string(__PRETTY_FUNCTION__) + ": " + devpath + " Slot" + std::to_string(Slot));

  ca_slot_info_t sinfo;
  sinfo.num = Slot;
  if (ioctl(fd, CA_GET_SLOT_INFO, &sinfo) != -1) {
     if ((sinfo.flags & CA_CI_MODULE_READY) != 0)
        return msReady;
     if ((sinfo.flags & CA_CI_MODULE_PRESENT) != 0)
        return msPresent;
     }
  else
     log(1, "CA_GET_SLOT_INFO failed on " + devpath + ": " + strerror(errno));

  return msNone;
}


bool cAdapter::Assign(cDevice* Device, bool Query) {
  _entering;

  return true;

  _leaving;
}
