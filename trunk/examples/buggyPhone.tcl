#!/bin/sh
# This line continues for Tcl, but is a single line for 'sh' \
exec wish "$0" -- ${1+"$@"}

package require Tk
package require iaxc

namespace eval ::buggyPhone {
	# registration data
	variable user ""
	variable pwd ""
	variable host ""

	variable state "idle"
}

proc ::buggyPhone::configPhone {} {
	set w .configPhone
	set title "buggyPhone - configuration"
	catch {destroy $w}
    set focus [focus]
	toplevel $w -relief solid -class TkSDialog

	wm title $w $title
	wm iconname $w $title
	wm transient $w [winfo toplevel [winfo parent $w]]

	set msg [message $w.msg -text "In order to use buggyPhone you've to specify the following \
		parameters to authenticate against your IAX provider" -width 400]
	set lblUser [label $w.lbluser -text "User"]
	set user [entry $w.user -bg white]
	set lblPwd [label $w.lblpwd -text "Password"]
	set pwd [entry $w.pwd -bg white]
	set lblHost [label $w.lblhost -text "Host"]
	set host [entry $w.host -bg white]

	set bts [frame $w.bts]
	set ok [button $bts.ok -text "Ok" -command {
		set ::buggyPhone::user [.configPhone.user get]
		set ::buggyPhone::pwd [.configPhone.pwd get]
		set ::buggyPhone::host [.configPhone.host get]
		destroy .configPhone
	}]
	set cancel [button $bts.cancel -text "Cancel" -command {destroy .configPhone}]

	grid $cancel $ok -sticky se -padx [list 3 0]

	grid $msg - -sticky news -pady 6 -padx 6
	grid $lblUser $user -sticky nsw -pady 6 -padx 6
	grid $lblPwd $pwd -sticky nsw -pady 6 -padx 6
	grid $lblHost $host -sticky nsw -pady 6 -padx 6
	grid $bts - -sticky se -padx 6 -pady 6
}

proc ::buggyPhone::call {phone} {
	if {$::buggyPhone::state != "idle"} {
		tk_dialog .bad "Error" "You cannot make a call if you aren't in a idle state" error 0 Ok
		return
	}
	set phoneNumber "$::buggyPhone::user:$::buggyPhone::pwd@$::buggyPhone::host/$phone"
	iaxcCall $phoneNumber
	set ::buggyPhone::state "calling"
	set ::buggyPhone::completed false
	
	while {!$::buggyPhone::completed} {
		foreach e [::iaxc::iaxcGetEvents] {
			set type [lindex $e 0]
			switch -exact -- $type { 
				"text" {
				   	set msg [lindex $e 3]
					.state delete 0 end
					.state insert 0 $msg
				}
				"levels" {
				}
				"state" {
					set sta [lindex $e 2]
					if {![expr $sta & $::iaxc::IAXC_CALL_STATE_ACTIVE]} {
						#call is NOT active (i.e. call's finished)
						set ::buggyPhone::completed true
						break
					}
				}
				"netstat" {
				}
				"url" {
				}
				"video" {
				}
				"registration" {
				}
			}
		}
		update
	}
	.hang invoke
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
entry .state -bg white

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
grid .state - - -sticky news

iaxcInit

update

if {$::buggyPhone::user eq "" || $::buggyPhone::pwd eq "" || $::buggyPhone::host eq ""} {
	::buggyPhone::configPhone
}

#set ::buggyPhone::user foo 
#set ::buggyPhone::pwd bar
#set ::buggyPhone::host "127.0.0.1"
