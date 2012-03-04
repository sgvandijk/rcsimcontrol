#!/bin/bash

# create temporary directory to work in
mkdir /tmp/rcs3d
mkdir /tmp/rcs3d/bin
mkdir /tmp/rcs3d/libs
mkdir /tmp/rcs3d/share

RCS3D=`which rcssserver3d`

# copy executable
cp $RCS3D /tmp/rcs3d/bin/

# copy libraries
ldd $RCS3d `spark-config --prefix`/lib/simspark/* `spark-config --prefix`/lib/rcssserver3d/* | perl -n -e '/(\/.+) \(0x/ && print "$1\n";' | sort | uniq | xargs -I '{}' cp '{}' /tmp/rcs3d/libs

cp -n `spark-config --prefix`/lib/simspark/* /tmp/rcs3d/libs
cp -n `spark-config --prefix`/lib/rcssserver3d/* /tmp/rcs3d/libs

# copy ruby files
cp -r `spark-config --prefix`/share/simspark /tmp/rcs3d/share
cp -r `spark-config --prefix`/share/rcssserver3d /tmp/rcs3d/share

# make run script
cat > /tmp/rcs3d/runrcs3d.sh <<DELIM
#!/bin/bash

mkdir ~/.simspark
cp -n share/rcssserver3d/* ~/.simspark -r
cp -n share/simspark/* ~/.simspark -r

cd bin
export LD_LIBRARY_PATH=../libs
./rcssserver3d --init-script-prefix ../share/simspark
DELIM
chmod 0755 /tmp/rcs3d/runrcs3d.sh

# tar it up
cd /tmp
tar czvf rcs3d.tar.gz rcs3d
