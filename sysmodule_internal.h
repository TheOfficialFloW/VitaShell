/**
 * \file
 * \brief Header file which defines system modules related variables and functions
 *
 * Copyright (C) 2015 PSP2SDK Project
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _PSP2_SYSMODULE_INTERNAL_H_
#define _PSP2_SYSMODULE_INTERNAL_H_

#include <psp2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* module IDs */
enum {
	SCE_SYSMODULE_PROMOTER_UTIL = 0x80000024
};

int sceSysmoduleLoadModuleInternal(SceUInt32 id);
int sceSysmoduleUnloadModuleInternal(SceUInt32 id);
int sceSysmoduleIsLoadedInternal(SceUInt32 id);

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_SYSMODULE_INTERNAL_H_ */