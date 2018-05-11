#!/bin/bash
SCRIPT_PATH="$(dirname "$0")"
export REGIONS="$(ls $SCRIPT_PATH/../../data/borders/Japan*.poly)"
#export REGIONS="$(ls $SCRIPT_PATH/../../data/borders/Japan*Nara.poly)"
#export PLANET="$SCRIPT_PATH/islands.o5m"
#export PLANET="/media/kiri/data/pbf/japan-latest.osm.pbf"
#export PLANET="/media/kiri/data/pbf/japan-latest.o5m"
#export PLANET="/media/kiri/data/pbf/japan-latest-nara.o5m"
#export PLANET="/media/kiri/data/pbf/japan-latest-contourtest.o5m"
#export PLANET="/media/kiri/data/pbf/japan-nara-contour-only.o5m"
#export PLANET="/media/kiri/data/pbf/japan-nara-contour-only-tagmod.o5m"
#export PLANET="/media/kiri/data/pbf/japan-latest-contourtest-nara.o5m"
#export PLANET="/media/kiri/data/pbf/japan-latest-nara-withContour.o5m"
export PLANET="/media/kiri/data/pbf/japan-latest-contour.o5m"
#export PLANET="/media/kiri/data/pbf/planet-latest.o5m"
export COASTS="/media/kiri/data/pbf/WorldCoasts.geom"
export SRTM_PATH="/media/kiri/data/SRTM/SRTM1v3.0/"
export TARGET="${TARGET:-$SCRIPT_PATH/target}"
export NS=raw
#export NS=map
#export NS=mem
bash "$SCRIPT_PATH/generate_planet.sh" $@
