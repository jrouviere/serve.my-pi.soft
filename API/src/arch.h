/*
 *  This file is part of OpenSCB project <http://openscb.org>.
 *  Copyright (C) 2010  Opendrain
 *
 *  OpenSCB software is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenSCB software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenSCB software.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARCH_H_
#define ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP16(a) ((((a)&0xFF00) >> 8) | (((a)&0xFF) << 8))
#define SWAP32(a) ( (((a)&0xFF000000) >> 24) | (((a)&0x00FF0000) >> 8) | \
                    (((a)&0x0000FF00) << 8)  | (((a)&0x000000FF) << 24))

#ifdef BIG_ENDIAN
/*for big endian processor*/
#define BE16(a) (a)
#define BE32(a) (a)
#else
/*for little endian processor*/
#define BE16(a) SWAP16(a)
#define BE32(a) SWAP32(a)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ARCH_H_ */
