#!/bin/sh
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig    
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs 

fi 

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
  #creating the folder tree
  mkdir ${OUTDIR}/rootfs
  cd ${OUTDIR}/rootfs
  mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
  mkdir usr/bin usr/lib usr/sbin
  mkdir -p var/log
  

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi


#echo "Completed install"
# TODO: Make and insatll busybox
#make menuconfig
#make ARCH= CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
	
echo "Completed install"	

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
${CROSS_COMPILE}gcc -print-sysroot
cd $SYSROOT
#ls -l lib/ld-linux-armhf.so.3
cd ${OUTDIR}/rootfs
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 lib
cp $SYSROOT/lib64/libc.so.6 lib64
cp $SYSROOT/lib64/libm.so.6 lib64
cp $SYSROOT/lib64/libresolv.so.2 lib64

echo "Added library dependencies to rootfs"

# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
FINDER_PATH=/home/jahnavi/Desktop/AESD/assignment-1-Jahnavi-pinnamaneni/finder-app
cd ${FINDER_PATH}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} writer

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_PATH}/writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp -r conf/ ${OUTDIR}/rootfs/home/conf
cp ${FINDER_PATH}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_PATH}/autorun-qemu.sh ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio
echo "Success"
