

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

