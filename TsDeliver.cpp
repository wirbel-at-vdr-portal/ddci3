/*******************************************************************************
 * @file TsDeliver.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include "TsReceiver.h"
#include "Logging.h"


/*******************************************************************************
 * class cDeliver
 ******************************************************************************/
cDeliver::cDeliver(cTsReceiver& Receiver, std::string& sec) :
  cThread(), receiver(Receiver), devpath(sec) {
  // don't use receiver in this function, unless you know what you are doing!

  SetDescription( "cDeliver %s", devpath.c_str());
  log(3, "cDeliver    " + devpath);
}


cDeliver::~cDeliver() {
  _entering;

  Cancel(3);

  _leaving;
}


bool cDeliver::Start(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "               " + devpath);

  return cThread::Start();
}


void cDeliver::Cancel(int waitSec) {
  _entering;

  cThread::Cancel(waitSec);

  _leaving;
}


void cDeliver::Action(void) {
  log(3, std::string(__PRETTY_FUNCTION__) + "      " + devpath);

  while(Running())
     receiver.Deliver();

  _leaving;
}
