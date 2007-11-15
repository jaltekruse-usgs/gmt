#!/bin/sh
#	$Id: generate.sh,v 1.8 2007-11-15 04:20:42 remko Exp $
#
# Tests project in generating lines

. ../functions.sh
header "Test project's ability to generate lines"

# First Cartesian lines
ps=generate.ps

# E-W line
project -C10/10 -A90 -G1 -L-9/11 -N > $$.xy
psxy -R0/25/0/25 -JX4i -P -K -X2i $$.xy -W2p,red > $ps
echo 21 11 12 0 1 LB E-W | pstext -R -J -O -K -Gred >> $ps

# 30 degrees azimuth
project -C5/5 -A30 -G1 -L-3/12 -N > $$.xy
psxy -R -J -O -K $$.xy -W2p,green >> $ps
echo 3 2 12 0 1 RT -A30 | pstext -R -J -O -K -Ggreen >> $ps

# Between two given points
project -C15/5 -E2/20 -G1 -N > $$.xy
psxy -R -J -O -K $$.xy -W2p,blue -B5g5WSne >> $ps
psxy -R -J -O -K -Sc0.1i -Gblue << EOF >> $ps
15	5
2	20
EOF

# SPherical test

# E-W line
project -C10/10 -A90 -G10 -L-5/15 > $$.xy
psxy -Rg -JA0/0/4i -O -K -Y4.5i $$.xy -W2p,red >> $ps
echo 27 11 12 0 1 LB E-W | pstext -R -J -O -K -Gred >> $ps
# 30 degrees azimuth
project -C5/5 -A30 -G10 -L-10/10 > $$.xy
psxy -R -J -O -K $$.xy -W2p,green >> $ps
echo -2 -3 12 0 1 RT -A30 | pstext -R -J -O -K -Ggreen >> $ps

# Between two given points
project -C15/5 -E-12/-30 -G10 > $$.xy
psxy -R -J -O -K $$.xy -W2p,blue >> $ps
psxy -R -J -O -K -Sc0.1i -Gblue << EOF >> $ps
15	5
-12	-30
EOF

# Point and rotation pole
project -C15/15 -T85/40 -G10 -L-20/60 > $$.xy
psxy -R -J -O -K $$.xy -W2p >> $ps
echo 15 15 | psxy -R -J -O -K -Sc0.1i -Gblack >> $ps
echo 85 40 | psxy -R -J -O -K -Sa0.1i -Gblack >> $ps
# The end
psbasemap -R -J -O -B30g30 >> $ps

pscmp
