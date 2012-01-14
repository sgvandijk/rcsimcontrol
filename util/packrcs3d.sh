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
ldd $RCS3D | awk '/=>/{ system("cp \"" $3 "\" /tmp/rcs3d/libs")}'
ldd `spark-config --prefix`/lib/simspark/* | awk '/=>/{ system("cp \"" $3 "\" /tmp/rcs3d/libs")}'
ldd `spark-config --prefix`/lib/rcssserver3d/* | awk '/=>/{ system("cp \"" $3 "\" /tmp/rcs3d/libs")}'
cp -d `spark-config --prefix`/lib/simspark/* /tmp/rcs3d/libs
cp -d `spark-config --prefix`/lib/rcssserver3d/* /tmp/rcs3d/libs

# copy ruby files
cp -r `spark-config --prefix`/share/simspark /tmp/rcs3d/share
cp -r `spark-config --prefix`/share/rcssserver3d /tmp/rcs3d/share

# make run script
cat > /tmp/rcs3d/runrcs3d.sh <<DELIM
#!/bin/bash

cd bin
export LD_LIBRARY_PATH=../libs
./rcssserver3d
DELIM
chmod 0755 /tmp/rcs3d/runrcs3d.sh

# tar it up
cd /tmp
tar czvf rcs3d.tar.gz rcs3d
