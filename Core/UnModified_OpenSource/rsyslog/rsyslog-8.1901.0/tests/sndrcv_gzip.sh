#!/bin/bash
# This test is similar to tcpsndrcv, but it forwards messages in
# zlib-compressed format (our own syslog extension).
# rgerhards, 2009-11-11
# This file is part of the rsyslog project, released under ASL 2.0
. ${srcdir:=.}/diag.sh init
export NUMMESSAGES=50000
empty_check() {
	if [ $(wc -l < "$RSYSLOG_OUT_LOG") -eq $NUMMESSAGES ]; then
		return 0
	fi
	return 1
}
export QUEUE_EMPTY_CHECK_FUNC=empty_check

#export RSYSLOG_DEBUG="debug nostdout noprintmutexaction"
export RSYSLOG_DEBUGLOG="log"
export RCVR_PORT="$(get_free_port)"
generate_conf
add_conf '
$ModLoad ../plugins/imtcp/.libs/imtcp
# then SENDER sends to this port (not tcpflood!)
$InputTCPServerRun '$RCVR_PORT'

$template outfmt,"%msg:F,58:2%\n"
$template dynfile,"'$RSYSLOG_OUT_LOG'" # trick to use relative path names!
:msg, contains, "msgnum:" ?dynfile;outfmt
'
startup

export RSYSLOG_DEBUGLOG="log2"
#valgrind="valgrind"
generate_conf 2
add_conf '
$ModLoad ../plugins/imtcp/.libs/imtcp
$InputTCPServerRun '$TCPFLOOD_PORT'

*.*	@@(z5)127.0.0.1:'$RCVR_PORT'
' 2
startup 2

# now inject the messages into instance 2. It will connect to instance 1,
# and that instance will record the data.
tcpflood -m$NUMMESSAGES
# shut down sender when everything is sent, receiver continues to run concurrently
shutdown_when_empty 2
wait_shutdown 2
# now it is time to stop the receiver as well
shutdown_when_empty
wait_shutdown

seq_check
exit_test
