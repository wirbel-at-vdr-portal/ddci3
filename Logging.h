/*******************************************************************************
 * @file Logging.h @brief Digital Devices Common Interface plugin for VDR.
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
#include "Common.h"


void log(int level, std::string msg);
void logEntering(std::string fname);
void logLeaving(std::string fname);

#define _entering if (LogLevel > 2) logEntering(__PRETTY_FUNCTION__)
#define _leaving  if (LogLevel > 2) logLeaving (__PRETTY_FUNCTION__)
