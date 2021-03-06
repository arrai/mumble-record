#! /bin/bash

cp murmur.ini murmur.ini.win32
cp murmur.ini murmur.ini.osx
cp murmur.ini murmur.ini.system

perl -pi -e 's/^(#|)ice=/ice=/' murmur.ini.win32

perl -pi -e 's/^(#|)ice=/ice=/' murmur.ini.osx
perl -pi -e 's/^(#|)dbus=/#dbus=/' murmur.ini.osx

perl -pi -e 's/^(#|)ice=/ice=/' murmur.ini.system
perl -pi -e 's/^(#|)dbus=.*$/dbus=system/' murmur.ini.system
perl -pi -e 's/^(#|)logfile=.*$/logfile=\/var\/log\/mumble-server\/mumble-server.log/' murmur.ini.system
perl -pi -e 's/^(#|)pidfile=.*$/pidfile=\/var\/run\/mumble-server\/mumble-server.pid/' murmur.ini.system
perl -pi -e 's/^(#|)database=.*$/database=\/var\/lib\/mumble-server\/mumble-server.sqlite/' murmur.ini.system
perl -pi -e 's/^(#|)uname=.*$/uname=mumble-server/' murmur.ini.system

perl -pi -e 'BEGIN{undef $/;} s/\n/\r\n/smg' murmur.ini.win32
