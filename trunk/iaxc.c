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
#define FIXSLEEP Sleep(250)
#else
#define FIXSLEEP usleep(250000)
#endif

#include "iaxc.h"

extern void tone_dtmf(char tone, int samples, double vol, short *data);

/* global cmd var... I'll try to put it away asap */
Tcl_Obj *callback_cmd;
Tcl_Obj *varName;
Tcl_Interp *interp_cmd;

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
	{"iaxcHangUp", "::iaxc::iaxcHangUp",iaxcHangUpCmd },
	{"iaxcSendDtmf", "::iaxc::iaxcSendDtmf",iaxcSendDtmfCmd },
	{"iaxcQuit", "::iaxc::iaxcQuit",iaxcQuitCmd },
	{NULL, NULL, NULL}
};


int
Iaxc_Init (Tcl_Interp *interp)
{
	iaxcCmd *cmdPtr;
	Tcl_Obj *codec_val, *codec_name;

	if (Tcl_InitStubs(interp, "8.3", 0) == NULL)
		return TCL_ERROR;


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

	Tcl_PkgProvide(interp, "iaxc", "0.1");
	return TCL_OK;
}

/*
 * Handle iax callback events
 */

int
callback(iaxc_event e)
{
	/* use the right interpreter
	 * i.e. the one used on init phase
	 */

	Tcl_Interp *interp=interp_cmd;
	Tcl_Obj *val, *key;
	int ret;

	/*
	 * set vars used in the supplied script
	 */

	val=Tcl_NewIntObj(e.type);
	key=Tcl_NewStringObj("etype", -1);
	Tcl_ObjSetVar2(interp, varName, key, val, 0);

	switch (e.type)
	{
		case IAXC_EVENT_TEXT:
			/*
			 * int type
			 * int callNo
			 * char message[IAXC_EVENT_BUFSIZ]
			 */
			key=Tcl_NewStringObj("type", -1);
			val=Tcl_NewIntObj(e.ev.text.type);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("callNo", -1);
			val=Tcl_NewIntObj(e.ev.text.callNo);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("message", -1);
			val=Tcl_NewStringObj(e.ev.text.message, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			break;
		case IAXC_EVENT_LEVELS:
			/*
			 * 	float input;
			 * 	float output;
			*/

			key=Tcl_NewStringObj("input", -1);
			val=Tcl_NewDoubleObj(e.ev.levels.input);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("output", -1);
			val=Tcl_NewDoubleObj(e.ev.levels.output);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

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
		
			key=Tcl_NewStringObj("callNo", -1);
			val=Tcl_NewIntObj(e.ev.call.callNo);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("state", -1);
			val=Tcl_NewIntObj(e.ev.call.state);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("format", -1);
			val=Tcl_NewIntObj(e.ev.call.format);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("remote", -1);
			val=Tcl_NewStringObj(e.ev.call.remote, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("remote_name", -1);
			val=Tcl_NewStringObj(e.ev.call.remote_name, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("local", -1);
			val=Tcl_NewStringObj(e.ev.call.local, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("local_context", -1);
			val=Tcl_NewStringObj(e.ev.call.local_context, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			break;
		case IAXC_EVENT_NETSTAT:
			/*
			 * int callNo;
			 * int rtt;
			 * struct iaxc_netstat local;
			 * struct iaxc_netstat remote;
			 */
			
			key=Tcl_NewStringObj("callNo", -1);
			val=Tcl_NewIntObj(e.ev.netstats.callNo);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("rtt", -1);
			val=Tcl_NewIntObj(e.ev.netstats.rtt);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			/* how can I report local and remote?? TODO */
			break;
		case IAXC_EVENT_URL:
			/*
			 * int callNo;
			 * int type;
			 * char url[IAXC_EVENT_BUFSIZ];
			 */

			key=Tcl_NewStringObj("callNo", -1);
			val=Tcl_NewIntObj(e.ev.url.callNo);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("type", -1);
			val=Tcl_NewIntObj(e.ev.url.type);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);
			
			key=Tcl_NewStringObj("url", -1);
			val=Tcl_NewStringObj(e.ev.url.url, -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			break;
		case IAXC_EVENT_VIDEO:
			/*
			 * int callNo;
			 * int format;
			 * int width;
			 * int height;
			 * unsigned char *data;
			 */

			key=Tcl_NewStringObj("callNo", -1);
			val=Tcl_NewIntObj(e.ev.video.callNo);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("format", -1);
			val=Tcl_NewIntObj(e.ev.video.format);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("width", -1);
			val=Tcl_NewIntObj(e.ev.video.width);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("height", -1);
			val=Tcl_NewIntObj(e.ev.video.height);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("data", -1);
			val=Tcl_NewStringObj((char *)(e.ev.video.data), -1);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			break;
		case IAXC_EVENT_REGISTRATION:
			/*
			 * int id;
			 * int reply;
			 * int msgcount;
			 */
			
			key=Tcl_NewStringObj("id", -1);
			val=Tcl_NewIntObj(e.ev.reg.id);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("reply", -1);
			val=Tcl_NewIntObj(e.ev.reg.reply);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			key=Tcl_NewStringObj("msgcount", -1);
			val=Tcl_NewIntObj(e.ev.reg.msgcount);
			Tcl_ObjSetVar2(interp, varName, key, val, 0);

			break;
	}


	/*
	 * execute the user's script; on error put a 
	 * message on stderr and return -1
	 */

	if ((ret=Tcl_EvalObjEx(interp, callback_cmd, 0)) != TCL_OK)
	{
		Tcl_Obj *res=Tcl_GetObjResult(interp);
		char *strres=Tcl_GetStringFromObj(res, NULL);
		perror(strres);
		return -1;
	}
	
	/* why 1?? copyed from iaxcli.c line 616*/
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
	
	if (objc > 5 || objc < 3)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "varName cmd ?audio type? ?nCalls?");
		return TCL_ERROR;
	}

	/*
	 * set default values for optional parameters
	 */
	
	int audType=AUDIO_INTERNAL_PA;
	int nCalls=1;
	
	/*
	 * store the varName into a global variable
	 */
	
	varName=Tcl_DuplicateObj(objv[1]);
	Tcl_IncrRefCount(varName);

	/*
	 * store the script to be execute on iax event
	 * handling on a global variable
	 */
	
	callback_cmd=Tcl_DuplicateObj(objv[2]);
	Tcl_IncrRefCount(callback_cmd);

	/*
	 * read optional parameters if any
	 */
	
	if (objc >= 4)
	{
		if(Tcl_GetIntFromObj(interp, objv[3], &audType) != TCL_OK)
			return TCL_ERROR;
	}

	if (objc == 5)
	{
		if(Tcl_GetIntFromObj(interp, objv[4], &nCalls) != TCL_OK)
			return TCL_ERROR;
	}

	/*
	 * init iaxclient lib
	 */
	
	/*if (iaxc_initialize(audType,nCalls))*/
	if (iaxc_initialize(AUDIO_INTERNAL_PA,1))
		return TCL_ERROR;

	iaxc_set_silence_threshold(-99.0); /* the default */
	iaxc_set_audio_output(0);   /* the default */
	interp_cmd=interp;
	iaxc_set_event_callback(callback);
	iaxc_start_processing_thread();

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
	return TCL_OK;
}
