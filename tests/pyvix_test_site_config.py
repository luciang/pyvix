

# This file contains some settings needed for testing pyvix propperly
# Please edit these values according to your setup.


# path to a .vmx that will be used in testing
# this vmx will be started/stopped/suspended
# it's snapshots will be deleted and/or created

generic_vmx = '/home/so/vmware/so-vm-debian-sid/faust.vmx'


# in some tests we need to connect to the guest operating system
# vmware tools need to be installed in the guest operating system


# Username and password for the guest account to use in testing
# (this need not be a priviledged root account)
guest_username = 'root'
guest_password = 'so'


# In some tests we need to place a file in a location on the guest
# system. This location should be writable by the guest account
# specified above.
# NOTE: Do not forget the trailling path separator!

guest_dest_dir = '/tmp/'
# guest_dest_dir = 'C:\\' # this would be good on Windows


# We need to test whether we can execute a file on the guest system.
# This specifies the file from the HOST that will be sent to the guest
# and executed. This file must exist in the ./data subdirectory.
guest_test_execution_program = 'pyvix_test_program__hello.py'
# guest_test_execution_program = 'pyvix_test_program__hello.bat' # this would be good on Windows




# For some unknown reason when I run some tests they result in a
# Segmentation Fault. The code that makes the invalid access is not
# pyvix code, but code from the libvmware-vix.so implementation.
#
# It usually happens while pyvix code is waiting in VixJob_Wait (but
# not necessary). libvmware-vix.so code is run on another thread and does the
# communication with vmware-serverd. The problem is in the libvmware-vix.so.

# Here's an example:
# Program received signal SIGSEGV, Segmentation fault.
# [Switching to Thread 0xb74dfb90 (LWP 6685)]
# 0xb773cffc in FoundryVMGuestOpToolsStateChangedHandler () from /usr/lib/libvmware-vix.so.0
# (gdb) bt
#0  0xb773cffc in FoundryVMGuestOpToolsStateChangedHandler () from /usr/lib/libvmware-vix.so.0
#1  0xb76fb241 in FoundryReportEventImpl () from /usr/lib/libvmware-vix.so.0
#2  0xb76faff1 in FoundryExecuteReportEvent () from /usr/lib/libvmware-vix.so.0
#3  0xb7715295 in FoundryAsyncOp_WorkerPollCallback () from /usr/lib/libvmware-vix.so.0
#4  0xb76ed39d in PollFire () from /usr/lib/libvmware-vix.so.0
#5  0xb76ec925 in PollFireAndDequeue () from /usr/lib/libvmware-vix.so.0
#6  0xb76ecd08 in PollExecuteDevice () from /usr/lib/libvmware-vix.so.0
#7  0xb76ebd41 in Poll_LoopTimeout () from /usr/lib/libvmware-vix.so.0
#8  0xb7715dab in FoundryAsyncOp_ThreadMainLoop () from /usr/lib/libvmware-vix.so.0
#9  0xb7723823 in FoundryThreadWrapperProc () from /usr/lib/libvmware-vix.so.0
#10 0xb7ec94c0 in start_thread () from /lib/i686/cmov/libpthread.so.0
#11 0xb7e196de in clone () from /lib/i686/cmov/libc.so.6

# this skips some of those tests
skip_tests_that_end_in_segmentation_fault = True



# again, another bug: if I try to authenticate to the host with wrong
# credentials (invalid username or password), I get an error code
# telling me of the mistake, BUT every other authentication call EVEN
# with correct credentials fails. I don't know if this is a problem in
# libvmware-vix.so or it's something I'm doing wrong.

skip_tests_bad_credentials_host_authentification = True

