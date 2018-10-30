#!/bin/bash

mkdir config_pkg
pushd config_pkg

cp ../../../data/classificator.txt ../../../data/types.txt ../../../data/colors.txt ./
zip -0j classificator_mod.zip classificator.txt types.txt colors.txt 
mv classificator_mod.zip classificator_mod_$(date -I).obb

mkdir styles
cp -r ../../../data/resources-* ./styles/                                                      
cp ../../../data/drules_proto.bin ../../../data/drules_proto_clear.bin ../../../data/drules_proto_dark.bin  ./styles/

popd

