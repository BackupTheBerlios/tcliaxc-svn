/* Copyright (c) 2006:
 *
 * Andrea 'sickpig' Suisani <sickpig@users.berlios.de>
 * Matteo 'bugant' Centenaro <bugant@users.berlios.de>
 *
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _TCLIAXC
#define _TCLIAXC

#ifdef BUILD_iaxc
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_iaxc */

/* os-dependent mutex macros */
#ifdef BUILD_iaxc
#define MUTEX CRITICAL_SECTION
#define MUTEXINIT(m) InitializeCriticalSection(m)
#define MUTEXLOCK(m) EnterCriticalSection(m)
#define MUTEXUNLOCK(m) LeaveCriticalSection(m)
#define MUTEXDESTROY(m) DeleteCriticalSection(m)

#else
#define MUTEX pthread_mutex_t
#define MUTEXINIT(m) pthread_mutex_init(m, NULL) //TODO: check error
#define MUTEXLOCK(m) pthread_mutex_lock(m)
#define MUTEXUNLOCK(m) pthread_mutex_unlock(m)
#define MUTEXDESTROY(m) pthread_mutex_destroy(m)
#endif /* BUILD_iaxc */

EXTERN int Iaxc_Init (Tcl_Interp *interp);
int iaxcInitCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcRegisterCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcAudioEncoding(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcCallCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcGetEvents(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcHangUpCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcSendDtmfCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int iaxcQuitCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif /* _TCLIAXC */
