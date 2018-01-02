/*
 * This file is part of Pako Bots division of Origami 3 Firmware.
 *
 *  Pako Bots division of Origami 3 Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots division of Origami 3 Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots division of Origami 3 Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdio.h>

#define PROPERTIES_STORAGE_KEY "props"

char * storage_get(char * store, char * key, char * value, size_t* len);
int storage_len(char * store, char * key, size_t* len);
int storage_set(char * store, char * key, char * value);
void storage_enable(void);
int storage_init_upgrade(size_t size);
int storage_upgrade(const void * data, size_t size);
int storage_fin_upgrade(void);

#endif
