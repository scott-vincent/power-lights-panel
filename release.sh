rel=v1.3.5
mkdir release >/dev/null 2>&1
rm -rf release/$rel >/dev/null 2>&1
mkdir release/$rel
cp power-lights-panel/power-lights-panel release/$rel
cp -rp power-lights-panel/settings release/$rel
sudo chown pi:pi release/$rel/settings/*.json
dos2unix release/$rel/settings/*.json
cp release/$rel/settings/default-settings.json release/$rel/settings/power-lights-panel.json
tar -zcvf release/power-lights-panel-$rel-raspi.tar.gz release/$rel
