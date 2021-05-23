/*******************************************************************************
 * @file TsDeliver.h @brief Digital Devices Common Interface plugin for VDR.
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
#pragma once
#include <string>
#include <vdr/thread.h>
#include <vdr/tools.h>

/*******************************************************************************
 * forward declarations.
 ******************************************************************************/
class cTsReceiver;


/*******************************************************************************
 * This class implements the deliver thread for the received CAM TS data.
 ******************************************************************************/
class cDeliver: public cThread {
private:
  cTsReceiver& receiver;  //< the CAM TS receiver with the buffer
  std::string devpath;    //< adapterX/ciY device path

public:
  /* Constructor, creates a new CAM TS deliver thread object.
   * @param Receiver - the CAM TS receiver with the buffer
   * @param sec      - device path for adapterX/secY
   */
  cDeliver(cTsReceiver& Receiver, std::string& sec);

  /* Destructor. */
  virtual ~cDeliver(void);

  bool Start(void);
  /* Executes the CAM TS receiver deliver function in a loop. */
  virtual void Action(void);
  void Cancel(int waitSec = 0);
};
