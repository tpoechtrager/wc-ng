#!/usr/bin/env bash

./src/build/package_nsis_windows_clang.sh
./src/build/package_linux64.sh

dir=$(date +%Y-%m-%d)
mkdir -p $dir

mv *$(./src/build/get_version.sh)* $dir
scp -r $dir root@dm-clan.k.vu:/srv/http/wc-ng/sauerbraten_2020/

echo ""
echo "https://wc-ng.mooo.com/sauerbraten_2020/$dir/"
echo ""

