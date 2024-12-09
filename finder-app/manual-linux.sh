#!/bin/bash
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
    ## start with clean build
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # configure build
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    ## to handle the error: multiple definition of `yylloc' 
    git apply ${FINDER_APP_DIR}/yylloc.patch

    # booting with QEMU
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}  all
    # build  kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # build device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "creating required directories"
mkdir -p ${OUTDIR}/rootfs
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp 
mkdir -p usr
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
echo "Busybox Installed"

cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
echo "Library dependendecnis read"

# TODO: Add library dependencies to rootfs
echo "Add library dependencis to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cd "${SYSROOT}"
cd "${OUTDIR}/rootfs"
sudo cp -L ${SYSROOT}/lib/ld-linux-aarch64.* lib
sudo cp -L ${SYSROOT}/lib64/libm.so.* lib64
sudo cp -L ${SYSROOT}/lib64/libresolv.so.* lib64
sudo cp -L ${SYSROOT}/lib64/libc.so.* lib64
echo "Completed Add libraries to rootfs"

# TODO: Make device nodes
if [ ! "${OUTDIR}/rootfs/dev/null" ]
then
    sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
fi

if [ ! "${OUTDIR}/rootfs/dev/console" ]
then
    sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
fi


# TODO: Clean and build the writer utility
echo $FINDER_APP_DIR
echo $CROSS_COMPILE 
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cd ${FINDER_APP_DIR}
cp finder.sh writer finder-test.sh writer.sh ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/home/conf
cp conf/* ${OUTDIR}/rootfs/home/conf
mkdir -p ${OUTDIR}/rootfs/conf
cp conf/* ${OUTDIR}/rootfs/conf
cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *


# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio

echo "All Done"