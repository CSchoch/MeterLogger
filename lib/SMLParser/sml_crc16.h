// Copyright 2011 Juri Glass, Mathias Runge, Nadim El Sayed
// DAI-Labor, TU-Berlin
//
// This file is part of libSML.
//
// libSML is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libSML is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libSML.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _SML_CRC16_H_
#define _SML_CRC16_H_

//#include "sml_shared.h"
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// CRC16 FSC implementation based on DIN 62056-46
uint16_t sml_crc16_calculate(unsigned char *cp, int len) ;

uint16_t crc16_ccitt(const unsigned char* data, uint32_t length);

#ifdef __cplusplus
}
#endif


#endif /* _SML_CRC16_H_ */
