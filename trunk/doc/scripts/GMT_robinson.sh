#!/bin/sh
#	$Id: GMT_robinson.sh,v 1.2 2004-04-10 17:19:14 pwessel Exp $
#

pscoast -R-180/180/-90/90 -JN0/4.5i -Bg30/g15 -Dc -A10000 -Ggray -P > GMT_robinson.ps
