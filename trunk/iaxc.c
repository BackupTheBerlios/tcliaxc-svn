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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <tcl.h>
#include <iaxclient.h>

#ifdef BUILD_iaxc
#include <windows.h>
#define FIXSLEEP Sleep(500)
#else
#include <pthread.h>
#define FIXSLEEP usleep(500000)
#endif

#include "iaxc.h"

extern void tone_dtmf(char tone, int samples, double vol, short *data);

MUTEX head_mutex;
Tcl_Obj *evt_list = NULL;

/*
 * Create iaxc's commands
 */

typedef struct {
	char *name;                 /* Name of command. */
	char *name2;                /* Name of command, in ::iaxc namespace. */
	Tcl_ObjCmdProc *objProc;    /* Command's object-based procedure. */
} iaxcCmd;

static iaxcCmd commands[] = {
	{"iaxcInit", "::iaxc::iaxcInit", iaxcInitCmd},
	{"iaxcRegister", "::iaxc::iaxcRegister", iaxcRegisterCmd},
	{"iaxcAudioEncoding", "::iaxc::iaxcAudioEncoding", iaxcAudioEncoding},
	{"iaxcCall", "::iaxc::iaxcCall", iaxcCallCmd},
	{"iaxcGetEvents", "::iaxc::iaxcGetEvents", iaxcGetEvents},
	{"iaxcHangUp", "::iaxc::iaxcHangUp",iaxcHangUpCmd },
	{"iaxcSendDtmf", "::iaxc::iaxcSendDtmf",iaxcSendDtmfCmd },
	{"iaxcQuit", "::iaxc::iaxcQuit",iaxcQuitCmd },
	{NULL, NULL, NULL}
};


int
Iaxc_Init (Tcl_Interp *interp)
{
	iaxcCmd *cmdPtr;
	Tcl_Obj *codec_val, *codec_name, *evt_val, *evt_name;

	if (Tcl_InitStubs(interp, "8.3", 0) == NULL)
		return TCL_ERROR;

	MUTEXINIT(&head_mutex);
	evt_list = Tcl_NewListObj(0, NULL);

	/* iaxc package commands */
	
	for (cmdPtr = commands; cmdPtr->name != NULL; cmdPtr++) {

		Tcl_CreateObjCommand(interp, cmdPtr->name, cmdPtr->objProc, (ClientData) "::",(Tcl_CmdDeleteProc *)NULL);
		Tcl_CreateObjCommand(interp, cmdPtr->name2, cmdPtr->objProc, (ClientData) "::iaxc::",(Tcl_CmdDeleteProc *)NULL);
	}
	
	
	if (Tcl_Eval(interp, "namespace eval ::iaxc namespace export *") == TCL_ERROR)
		return TCL_ERROR;

	/*
	 * set available codecs on ::iaxc namespace
	 */

	codec_val = Tcl_NewIntObj(1 << 1);
	codec_name = Tcl_NewStringObj("::iaxc::IAXC_FORMAT_GSM", -1);
	Tcl_ObjSetVar2(interp, codec_name, NULL, codec_val, 0);
	
	codec_val = Tcl_NewIntObj(1 << 2);
	codec_name = Tcl_NewStringObj("::iaxc::IAXC_FORMAT_ULAW", -1);
	Tcl_ObjSetVar2(interp, codec_name, NULL, codec_val, 0);
	
	codec_val = Tcl_NewIntObj(1 << 3);
	codec_name = Tcl_NewStringObj("::iaxc::IAXC_FORMAT_ALAW", -1);
	Tcl_ObjSetVar2(interp, codec_name, NULL, codec_val, 0);
	
	codec_val = Tcl_NewIntObj(1 << 9);
	codec_name = Tcl_NewStringObj("::iaxc::IAXC_FORMAT_SPEEX", -1);
	Tcl_ObjSetVar2(interp, codec_name, NULL, codec_val, 0);
	
	codec_val = Tcl_NewIntObj(1 << 10);
	codec_name = Tcl_NewStringObj("::iaxc::IAXC_FORMAT_ILBC", -1);
	Tcl_ObjSetVar2(interp, codec_name, NULL, codec_val, 0);

	/*
	 * set available event types
	 */
	
	evt_val = Tcl_NewStringObj("text", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_TEXT", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("levels", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_LEVELS", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("state", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_STATE", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("netstat", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_NETSTAT", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("url", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_URL", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("video", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_VIDEO", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);
	
	evt_val = Tcl_NewStringObj("registration", -1);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_EVENT_REGISTRATION", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	/*
	 * available states for a call (contained in an state event)
	 */
	
	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_FREE);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_FREE", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_ACTIVE);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_ACTIVE", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_OUTGOING);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_OUTGOING", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_RINGING);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_RINGING", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_COMPLETE);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_COMPLETE", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_SELECTED);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_SELECTED", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_BUSY);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_BUSY", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	evt_val = Tcl_NewIntObj(IAXC_CALL_STATE_TRANSFER);
	evt_name = Tcl_NewStringObj("::iaxc::IAXC_CALL_STATE_TRANSFER", -1);
	Tcl_ObjSetVar2(interp, evt_name, NULL, evt_val, 0);

	Tcl_PkgProvide(interp, "iaxc", "0.1");
	return TCL_OK;
}

/*
 * Handle iax callback events
 */

int
callback(iaxc_event e)
{
	Tcl_Obj *elm_list = NULL, *val[8];

	switch (e.type)
	{
		case IAXC_EVENT_TEXT:
			/*
			 * int type
			 * int callNo
			 * char message[IAXC_EVENT_BUFSIZ]
			 */

			val[0] = Tcl_NewStringObj("text", -1);
			val[1] = Tcl_NewIntObj(e.ev.text.type);
			val[2] = Tcl_NewIntObj(e.ev.text.callNo);
			val[3] = Tcl_NewStringObj(e.ev.text.message, -1);

			elm_list = Tcl_NewListObj(4, val);
			break;
		case IAXC_EVENT_LEVELS:
			/*
			 * 	float input;
			 * 	float output;
			 */

			val[0] = Tcl_NewStringObj("levels", -1);
			val[1] = Tcl_NewDoubleObj(e.ev.levels.input);
			val[2] = Tcl_NewDoubleObj(e.ev.levels.output);

			elm_list = Tcl_NewListObj(3, val);
			break;
		case IAXC_EVENT_STATE:
			/*
			 *     int callNo;
			 *     int state;
			 *     int format;
			 *     char remote[IAXC_EVENT_BUFSIZ];
			 *     char remote_name[IAXC_EVENT_BUFSIZ];
			 *     char local[IAXC_EVENT_BUFSIZ];
			 *     char local_context[IAXC_EVENT_BUFSIZ];
			 */

			val[0] = Tcl_NewStringObj("state", -1);
			val[1] = Tcl_NewIntObj(e.ev.call.callNo);
			val[2] = Tcl_NewIntObj(e.ev.call.state);
			val[3] = Tcl_NewIntObj(e.ev.call.format);
			val[4] = Tcl_NewStringObj(e.ev.call.remote, -1);
			val[5] = Tcl_NewStringObj(e.ev.call.remote_name, -1);
			val[6] = Tcl_NewStringObj(e.ev.call.local, -1);
			val[7] = Tcl_NewStringObj(e.ev.call.local_context, -1);

			elm_list = Tcl_NewListObj(8, val);
			break;
		case IAXC_EVENT_NETSTAT:
			/*
			 * int callNo;
			 * int rtt;
			 * struct iaxc_netstat local;
			 * struct iaxc_netstat remote;
			 */

			val[0] = Tcl_NewStringObj("netstat", -1);
			val[1] = Tcl_NewIntObj(e.ev.netstats.callNo);
			val[2] = Tcl_NewIntObj(e.ev.netstats.rtt);
			/* how can I report local and remote?? TODO */

			elm_list = Tcl_NewListObj(3, val);
			break;
		case IAXC_EVENT_URL:
			/*
			 * int callNo;
			 * int type;
			 * char url[IAXC_EVENT_BUFSIZ];
			 */

			val[0] = Tcl_NewStringObj("url", -1);
			val[1] = Tcl_NewIntObj(e.ev.url.callNo);
			val[2] = Tcl_NewIntObj(e.ev.url.type);
			val[3] = Tcl_NewStringObj(e.ev.url.url, -1);

			elm_list = Tcl_NewListObj(4, val);
			break;
		case IAXC_EVENT_VIDEO:
			/*
			 * int callNo;
			 * int format;
			 * int width;
			 * int height;
			 * unsigned char *data;
			 */

			val[0] = Tcl_NewStringObj("video", -1);
			val[1] = Tcl_NewIntObj(e.ev.video.callNo);
			val[2] = Tcl_NewIntObj(e.ev.video.format);
			val[3] = Tcl_NewIntObj(e.ev.video.width);
			val[4] = Tcl_NewIntObj(e.ev.video.height);
			val[5] = Tcl_NewStringObj((char *)(e.ev.video.data), -1);

			elm_list = Tcl_NewListObj(6, val);
			break;
		case IAXC_EVENT_REGISTRATION:
			/*
			 * int id;
			 * int reply;
			 * int msgcount;
			 */

			val[0] = Tcl_NewStringObj("registration", -1);
			val[1] = Tcl_NewIntObj(e.ev.reg.id);
			val[2] = Tcl_NewIntObj(e.ev.reg.reply);
			val[3] = Tcl_NewIntObj(e.ev.reg.msgcount);

			elm_list = Tcl_NewListObj(4, val);
			break;
	}

	if (elm_list != NULL)
	{	
		MUTEXLOCK(&head_mutex);
		Tcl_ListObjAppendElement(NULL, evt_list, elm_list);
		MUTEXUNLOCK(&head_mutex);
	}

	return 1;
}

/*
 * initialize iaxclient lib
 */

int
iaxcInitCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	/*
	 * user can specify 4 args, the first 2 are required:
	 * varName a var to store iax's event
	 * cmd a tcl user's script to execute
	 * audio_type [optional] unknown
	 * nCalls [optional] unknown
	 */
	
	if (objc > 3)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "?audio type? ?nCalls?");
		return TCL_ERROR;
	}

	/*
	 * set default values for optional parameters
	 */
	
	int audType=AUDIO_INTERNAL_PA;
	int nCalls=1;
	
	/*
	 * read optional parameters if any
	 */
	
	if (objc >= 2)
	{
		if(Tcl_GetIntFromObj(interp, objv[1], &audType) != TCL_OK)
			return TCL_ERROR;
	}

	if (objc == 3)
	{
		if(Tcl_GetIntFromObj(interp, objv[2], &nCalls) != TCL_OK)
			return TCL_ERROR;
	}

	/*
	 * init iaxclient lib
	 */
	
	/*if (iaxc_initialize(audType,nCalls))*/
	if (iaxc_initialize(AUDIO_INTERNAL_PA,1) == -1)
		return TCL_ERROR;

	iaxc_set_silence_threshold(-99.0); /* the default */
	iaxc_set_audio_output(0);   /* the default */
	iaxc_set_event_callback(callback);
	if (iaxc_start_processing_thread())
		return TCL_ERROR;

	return TCL_OK;
}

/*
 * register a user to * server providing
 * the supplied credentials (user,pw,host).
 */

int
iaxcRegisterCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	if (objc != 4)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "user pass host");
		return TCL_ERROR;
	}

	char *user;
	char *pass;
	char *host;

	user=Tcl_GetString(objv[1]);
	pass=Tcl_GetString(objv[2]);
	host=Tcl_GetString(objv[3]);

	iaxc_register(user, pass, host);

	return TCL_OK;
}


/*
 * set audio encoding format
 */

int
iaxcAudioEncoding (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	int allowed_codecs = IAXC_FORMAT_GSM|IAXC_FORMAT_ULAW|IAXC_FORMAT_ALAW|IAXC_FORMAT_SPEEX|IAXC_FORMAT_ILBC;

	if (objc != 2)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "format");
		return TCL_ERROR;
	}
	int codec;
	Tcl_GetIntFromObj(interp, objv[1], &codec);

	iaxc_set_formats(codec, allowed_codecs);

	return TCL_OK;
}

/*
 * dial the specified phone number.
 * (the number MUST be supplied as
 * iax protocol require; i.e. user:pwd@host/phone_number)
 */

int
iaxcCallCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	if (objc != 2)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "phoneNumber");
		return TCL_ERROR;
	}
	char *phoneNumber;
	Tcl_Obj *phone=objv[1];
    phoneNumber=Tcl_GetStringFromObj(phone, NULL);

	iaxc_call(phoneNumber);

	/* 
	 * sleeps for 25ms to avoid annoing iaxclient
	 * segfault. (see BUGS)
	 */

	FIXSLEEP;
	
	return TCL_OK;
}

int
iaxcGetEvents(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Obj *retList;
	
	if (objc != 1)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "");
		return TCL_ERROR;
	}
	
	MUTEXLOCK(&head_mutex);
	retList = evt_list;
	evt_list = Tcl_NewListObj(0, NULL);
	MUTEXUNLOCK(&head_mutex);

	Tcl_SetObjResult(interp, retList);

	return TCL_OK;
}

/*
 * hang up (any comment needed?)
 */

int
iaxcHangUpCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	if (objc != 1)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "");
		return TCL_ERROR;
	}

	if (iaxc_selected_call() >= 0)
		iaxc_dump_call();

	/* 
	 * sleeps for 25ms to avoid annoing iaxclient
	 * segfault. (see BUGS)
	 */

	FIXSLEEP;
	
	return TCL_OK;
}

/*
 * send tone 
 */


int
iaxcSendDtmfCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	if (objc != 2)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "digit");
		return TCL_ERROR;
	}

	char *digita=Tcl_GetString(objv[1]);
	char digit=*digita;
	/*memcpy(&digit, digita, sizeof(char));*/

	struct iaxc_sound tone;
	short *buff_tone=malloc(sizeof(short)*2000);

	/*
	 * send the sound out the the connection's other side
	 */

	/*iaxc_send_dtmf(digit);*/

	/*
	 * play the tone back to the user
	 */

	tone.data=buff_tone;
	tone.len = 2000;
	tone.malloced=0;
	tone.repeat=0;
	tone_dtmf(digit,1600,100.0, buff_tone);
	tone_dtmf('X',400,100.0, buff_tone+1600);
	iaxc_play_sound(&tone, 0);

	return TCL_OK;
}

/*
 * reset library handler.
 */

int
iaxcQuitCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	iaxc_shutdown();
	iaxc_stop_processing_thread();
	
	MUTEXLOCK(&head_mutex);
	Tcl_DecrRefCount(evt_list);
	MUTEXUNLOCK(&head_mutex);
	MUTEXDESTROY(&head_mutex);

	return TCL_OK;
}
