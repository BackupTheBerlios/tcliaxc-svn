#!/bin/sh
# This line continues for Tcl, but is a single line for 'sh' \
exec wish "$0" -- ${1+"$@"}

lappend auto_path .
package require Tk
package require iaxc

namespace eval ::buggyPhone {
	# registration data
	variable user ""
	variable pwd ""
	variable host ""

	variable state "idle"
}

proc ::buggyPhone::call {phone} {
	if {$::buggyPhone::state != "idle"} {
		tk_dialog .bad "Error" "You cannot make a call if you aren't in a idle state" error 0 Ok
		return
	}
	set phoneNumber "$::buggyPhone::user:$::buggyPhone::pwd@$::buggyPhone::host/$phone"
	iaxcCall $phoneNumber
	set ::buggyPhone::state "calling"
}

proc ::buggyPhone::sendTone {digit} {
	if {$::buggyPhone::state != "calling"} {
		tk_dialog .bad "Error" "You cannot send a dtmf tone while not calling" error 0 Ok
		return
	}
	
	iaxcSendDtmf $digit
}

proc ::buggyPhone::digitPress {digit} {
	.keyboard.$digit configure -relief sunken
	update
	if {$::buggyPhone::state == "calling"} {
		::buggyPhone::sendTone $digit
	} else {
		.phone insert end $digit
	}
	after 200 .keyboard.$digit configure -relief raised
}

entry .phone -bg white
button .call -text "Call" -command {::buggyPhone::call [.phone get]}
button .hang -text "Hangup" -command {iaxcHangUp; set ::buggyPhone::state "idle"}

frame .keyboard

set keys [list]

for {set i 0} {$i <= 9} {incr i} {
	lappend keys $i
}

lappend keys "*"
lappend keys "#"

foreach k $keys {
	set cmd "::buggyPhone::digitPress $k"
	button .keyboard.$k -text $k -command $cmd -relief raised
	switch -regexp -- $k {
		[0-9] {
		   	bind . <KeyPress-$k> $cmd
		}
	}
}

bind . <KeyPress-asterisk> {::buggyPhone::digitPress *}

bind . <BackSpace> {.phone delete [expr [string length [.phone get]] -1 ] end}

grid .keyboard.1 .keyboard.2 .keyboard.3 -sticky news -padx 5 -pady 5
grid .keyboard.4 .keyboard.5 .keyboard.6 -sticky news -padx 5 -pady 5
grid .keyboard.7 .keyboard.8 .keyboard.9 -sticky news -padx 5 -pady 5
grid .keyboard.* .keyboard.0 .keyboard.# -sticky news -padx 5 -pady 5

grid .phone .call .hang -sticky news
grid .keyboard - - -sticky news 

iaxcInit type {}

set ::buggyPhone::user foo 
set ::buggyPhone::pwd bar
set ::buggyPhone::host "127.0.0.1"
