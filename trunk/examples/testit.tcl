package require iaxc

puts "start test"
set cmd {
	set iaxc_state_free 0
	set iaxc_state_active [expr 1<<1]
	set iaxc_state_outgoing [expr 1<<2]
	set iaxc_state_ringing [expr 1<<3]
	set iaxc_state_complete [expr 1<<4]
	set iaxc_state_selected [expr 1<<5]
	set iaxc_state_busy [expr 1<<6]
	set iaxc_state_transfer [expr 1<<7]

	set states [list $iaxc_state_free $iaxc_state_active $iaxc_state_outgoing \
		$iaxc_state_ringing $iaxc_state_complete $iaxc_state_selected $iaxc_state_busy \
		$iaxc_state_transfer]
	set names [list "free" "active" "outgoing" "ringing" "complete" "selected" "busy" \
		"transfer"]

	switch -- $type(etype) {
		1 {
			#TEXT
			#puts "type: $type(type)"
			puts "callNo: $type(callNo)"
			puts "message: $type(message)"
		}
		3 {
			#CALL_STATE
			puts "callNo: $type(callNo)"
			set strState ""
			foreach s $states {
				if {[expr $type(state) & $s] != 0} {
					set stateName [lindex $names [lsearch $states $s]]
					set strState "$strState$stateName "
				}
			}
			puts "state: $strState --- $type(state)"
			#puts "format: $type(format)"
			#puts "remote: $type(remote)"
			#puts "remote_name: $type(remote_name)"
			#puts "local: $type(local)"
			#puts "local_context: $type(local_context)"
		}
	}
}

iaxcInit type $cmd
puts "iaxcInit done"

set user foo
set pwd bar
set host "127.0.0.1"
set phone "000000000000000000000"

puts "register..."
iaxcRegister $user $pwd $host

set phoneNumber "$user:$pwd@$host/$phone"
puts "calling $phoneNumber"
iaxcCall $phoneNumber
