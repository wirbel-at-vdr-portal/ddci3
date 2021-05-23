/*******************************************************************************
 * @file Common.cpp @brief Digital Devices Common Interface plugin for VDR.
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
#include <vdr/remux.h>   // TS_SIZE, TS_SYNC_BYTE
#include "Common.h"


uint8_t* CheckTsSync(uint8_t* data, int length, int& skipped) {
  skipped = 0;
  if (*data != TS_SYNC_BYTE) {
     skipped = 1;

     while(skipped < length &&
          (data[skipped] != TS_SYNC_BYTE || (((length - skipped) > TS_SIZE) &&
          (data[skipped + TS_SIZE] != TS_SYNC_BYTE))))
        skipped++;
     }
  return data + skipped;
}


bool CheckAllSync(uint8_t* data, int length, uint8_t*&  posnsync) {
  posnsync = nullptr;
  length -= length % TS_SIZE;
  int len = 0;

  while((len < length) && data[len] == TS_SYNC_BYTE)
     len += TS_SIZE;

  bool ret = (len != length);
  if (!ret)
    posnsync = &data[len];
  return ret;
}
