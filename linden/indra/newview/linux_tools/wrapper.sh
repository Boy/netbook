#!/bin/bash

## Here are some configuration options for Linux Client Testers.
## These options are for self-assisted troubleshooting during this beta
## testing phase; you should not usually need to touch them.

## - Avoids using the ESD audio driver.
#export LL_BAD_ESD=x

## - Avoids using the OSS audio driver.
#export LL_BAD_OSS=x

## - Avoids using the ALSA audio driver.
#export LL_BAD_ALSA=x

## - Avoids the optional OpenGL extensions which have proven most problematic
##   on some hardware.  Disabling this option may cause BETTER PERFORMANCE but
##   may also cause CRASHES and hangs on some unstable combinations of drivers
##   and hardware.
export LL_GL_BASICEXT=x

## - Avoids *all* optional OpenGL extensions.  This is the safest and least-
##   exciting option.  Enable this if you experience stability issues, and
##   report whether it helps in the Linux Client Testers forum.
#export LL_GL_NOEXT=x

## - For advanced troubleshooters, this lets you disable specific GL
##   extensions, each of which is represented by a letter a-o.  If you can
##   narrow down a stability problem on your system to just one or two
##   extensions then please post details of your hardware (and drivers) to
##   the Linux Client Testers forum along with the minimal
##   LL_GL_BLACKLIST which solves your problems.
#export LL_GL_BLACKLIST=abcdefghijklmno


## Everything below this line is just for advanced troubleshooters.
##-------------------------------------------------------------------

## - For advanced debugging cases, you can run the viewer under the
##   control of another program, such as strace, gdb, or valgrind.  If
##   you're building your own viewer, bear in mind that the executable
##   in the bin directory will be stripped: you should replace it with
##   an unstripped binary before you run.
#export LL_WRAPPER='gdb --args'
#export LL_WRAPPER='valgrind --smc-check=all --log-file=secondlife.vg --leak-check=full --suppressions=/usr/lib/valgrind/glibc-2.5.supp --suppressions=secondlife-i686.supp'

## - Avoids an often-buggy X feature that doesn't really benefit us anyway.
export SDL_VIDEO_X11_DGAMOUSE=0

## - Works around a problem with misconfigured 64-bit systems not finding GL
export LIBGL_DRIVERS_PATH="${LIBGL_DRIVERS_PATH}":/usr/lib64/dri:/usr/lib32/dri:/usr/lib/dri

## - The 'scim' GTK IM module widely crashes the viewer.  Avoid it.
if [ "$GTK_IM_MODULE" = "scim" ]; then
    export GTK_IM_MODULE=xim
fi

## - Work around the ATI mouse cursor crash bug with fglrx and Mobility Radeon:
##   If you don't have lspci, or if you think this can solve problems with a non-mobility Radeon
##   card, you may simply uncomment the following line to force the work around:
#export LL_ATI_MOUSE_CURSOR_BUG=x
if [ -x /usr/bin/lspci ] ; then
	if lspci | grep "Mobility Radeon" &>/dev/null ; then
		if lsmod | grep fglrx &>/dev/null ; then
			export LL_ATI_MOUSE_CURSOR_BUG=x
		fi
	fi
fi

## Nothing worth editing below this line.
##-------------------------------------------------------------------

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
cd "${RUN_PATH}"

# Re-register the secondlife:// protocol handler every launch, for now.
./register_secondlifeprotocol.sh

if [ -n "$LL_TCMALLOC" ]; then
    tcmalloc_libs='/usr/lib/libtcmalloc.so.0 /usr/lib/libstacktrace.so.0 /lib/libpthread.so.0'
    all=1
    for f in $tcmalloc_libs; do
        if [ ! -f $f ]; then
	    all=0
	fi
    done
    if [ $all != 1 ]; then
        echo 'Cannot use tcmalloc libraries: components missing' 1>&2
    else
	export LD_PRELOAD=$(echo $tcmalloc_libs | tr ' ' :)
	if [ -z "$HEAPCHECK" -a -z "$HEAPPROFILE" ]; then
	    export HEAPCHECK=${HEAPCHECK:-normal}
	fi
    fi
fi

export SL_ENV='LD_LIBRARY_PATH="`pwd`"/lib:"`pwd`"/app_settings/mozilla-runtime-linux-i686:"${LD_LIBRARY_PATH}"'
export SL_CMD='$LL_WRAPPER bin/do-not-directly-run-secondlife-bin'
export SL_OPT="`cat gridargs.dat` $@"

# Run the program
eval ${SL_ENV} ${SL_CMD} ${SL_OPT} || LL_RUN_ERR=runerr

# Handle any resulting errors
if [ -n "$LL_RUN_ERR" ]; then
	LL_RUN_ERR_MSG=""
	if [ "$LL_RUN_ERR" = "runerr" ]; then
		# generic error running the binary
		echo '*** Unclean shutdown. ***'
		if [ "`arch`" = "x86_64" ]; then
			echo
			cat << EOFMARKER
You are running the Second Life Viewer on a x86_64 platform.  The
most common problems when launching the Viewer (particularly
'bin/do-not-directly-run-secondlife-bin: not found' and 'error while
loading shared libraries') may be solved by installing your Linux
distribution's 32-bit compatibility packages.
For example, on Ubuntu and other Debian-based Linuxes you might run:
$ sudo apt-get install ia32-libs ia32-libs-gtk ia32-libs-kde ia32-libs-sdl
EOFMARKER
		fi
	fi
fi
	

echo
echo '*******************************************************'
echo 'This is a BETA release of the Second Life linux client.'
echo 'Thank you for testing!'
echo 'Please see README-linux.txt before reporting problems.'
echo
