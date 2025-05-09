#!/bin/bash

vmdir="/root/atc25ShieldReduce/Dataset/redundant" # set to current dir
curdir="/root/atc25ShieldReduce/Dataset/redundant"
a=0.3     # Percentage of files to modify
b=0.3     # Percentage of content to modify in each file
M=10000   # Target size for new files in KB for rrandomio.py
vers=30
users=1

# Ensure the base directory for mount point and symlink target exists
sudo mkdir -p $vmdir/root

# Create/update symbolic link to point to the mount point
if [ ! -L "${curdir}/synthetic_fs" ] || [ "$(readlink ${curdir}/synthetic_fs)" != "${vmdir}/root" ]; then
    sudo ln -sfn $vmdir/root ${curdir}/synthetic_fs
fi

for((i=1;i<=${users};i++)); do
    echo "Processing user $i..."
    cd $vmdir # Consistent working directory for loop device operations

    # --- Cleanup before setting up image for this user iteration ---
    if sudo mount | grep -q "$vmdir/root"; then
        sudo umount $vmdir/root
    fi
    if sudo kpartx -l /dev/loop3 2>/dev/null | grep -q "loop3p"; then
        sudo kpartx -d /dev/loop3
    fi
    if sudo losetup -a | grep -q "/dev/loop3"; then
        sudo losetup -d /dev/loop3
    fi
    sudo rm -f $curdir/vmroot.raw

    # --- Image setup ---
    echo "Converting qcow2 to raw..."
    sudo qemu-img convert -f qcow2 $curdir/CentOS-7-x86_64-Azure-1704.qcow2 -O raw $curdir/vmroot.raw
    if [ $? -ne 0 ]; then echo "qemu-img convert failed"; exit 1; fi

    echo "Setting up loop device..."
    sudo losetup /dev/loop3 $curdir/vmroot.raw
    if [ $? -ne 0 ]; then echo "losetup failed"; sudo rm -f $curdir/vmroot.raw; exit 1; fi

    echo "Mapping partitions..."
    sudo kpartx -a /dev/loop3
    if [ $? -ne 0 ]; then echo "kpartx -a failed"; sudo losetup -d /dev/loop3; sudo rm -f $curdir/vmroot.raw; exit 1; fi

    sleep 2s # Allow time for device nodes

    # --- Mount the partition ---
    echo "Mounting /dev/mapper/loop3p1 to $vmdir/root..."
    # Ensure the partition device exists
    if [ ! -b /dev/mapper/loop3p1 ]; then
        echo "Error: /dev/mapper/loop3p1 not found!"
        sudo kpartx -d /dev/loop3; sudo losetup -d /dev/loop3; sudo rm -f $curdir/vmroot.raw
        exit 1
    fi
    sudo mount /dev/mapper/loop3p1 $vmdir/root
    if [ $? -ne 0 ]; then
        echo "Mount failed for /dev/mapper/loop3p1 on $vmdir/root"
        sudo kpartx -d /dev/loop3; sudo losetup -d /dev/loop3; sudo rm -f $curdir/vmroot.raw
        exit 1
    fi

    cd $curdir # Switch to script's base directory to run helper scripts
    for((j=1;j<=${vers};j++)); do
        echo "Processing version $j for user $i..."

        # Modify files on the mounted filesystem via the symlink
        echo "Running rrandomio.py..."
        sudo python2 ${curdir}/rrandomio.py "${curdir}/synthetic_fs/" $a $b $M

        # Perform the backup
        echo "Running full_backup.sh..."
        # mkdir -p ${curdir}/test1 # Ensure backup destination directory exists
        sudo bash ${curdir}/full_backup.sh $i $j

        echo "Completed version $j for user $i."
    done

    # --- Cleanup after this user's versions are processed ---
    echo "Cleaning up for user $i..."
    cd $vmdir # Consistent working directory
    if sudo mount | grep -q "$vmdir/root"; then
        sudo umount $vmdir/root
    fi
    echo "User $i processing complete."
done

# --- Final script cleanup ---
echo "Starting final cleanup..."
cd $vmdir

# Final unmount, kpartx, losetup detachment, and raw image removal
if sudo mount | grep -q "$vmdir/root"; then
    sudo umount $vmdir/root
fi
if sudo kpartx -l /dev/loop3 2>/dev/null | grep -q "loop3p"; then
    sudo kpartx -d /dev/loop3
fi
if sudo losetup -a | grep -q "/dev/loop3"; then
    sudo losetup -d /dev/loop3
fi
sudo rm -f $curdir/vmroot.raw

# Remove the symlink
sudo rm -f ${curdir}/synthetic_fs

# Optionally, remove the mount point directory if it's no longer needed for other purposes
# sudo rmdir $vmdir/root # Only if empty
# sudo rmdir $vmdir     # Only if empty

echo "Backup routine complete."
