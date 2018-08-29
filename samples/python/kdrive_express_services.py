# -*- coding: utf-8 -*-

#
# Copyright (c) 2002-2016 WEINZIERL ENGINEERING GmbH
# All rights reserved.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY,
# WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
#

from ctypes import (
    CDLL, CFUNCTYPE,
    create_string_buffer,
    c_int, c_void_p,
    c_ushort, c_uint, c_ubyte,
    byref
)


# load the kdriveExpress dll (windows)
# for linux replace with kdriveExpress.so
kdrive = CDLL('kdriveExpress.dll')

# the error callback pointer to function type
ERROR_CALLBACK = CFUNCTYPE(None, c_int, c_void_p)

# defines from kdrive (not available from the library)
KDRIVE_INVALID_DESCRIPTOR = -1
KDRIVE_ERROR_NONE = 0
KDRIVE_LOGGER_FATAL = 1
KDRIVE_LOGGER_INFORMATION = 6
SERIAL_NUMBER_LENGTH = 6

# The maximum number of individual addresses we expect to read at one time
MAX_IND_ADDR = 5

# Set to 1 to use connection-oriented
# Set to 0 for connection-less
connection_oriented = 0

# The address of the device that we connect
# to for the device services (Property Value Read etc)
address = c_ushort(0x1165)


def main():
    # Configure the logging level and console logger
    kdrive.kdrive_logger_set_level(KDRIVE_LOGGER_INFORMATION)
    kdrive.kdrive_logger_console()

    # We register an error callback as a convenience logger function to
    # print out the error message when an error occurs.
    error_callback = ERROR_CALLBACK(on_error_callback)
    kdrive.kdrive_register_error_callback(error_callback, None)
    
    # We create a Access Port descriptor. This descriptor is then used for
    # all calls to that specific access port.
    ap = open_access_port()
    
    # We check that we were able to allocate a new descriptor
    # This should always happen, unless a bad_alloc exception is internally thrown
    # which means the memory couldn't be allocated, or there are no usb ports available
    # or we were otherwise unable to open the port
    if ap == KDRIVE_INVALID_DESCRIPTOR:
        kdrive.kdrive_logger(KDRIVE_LOGGER_FATAL, 'Unable to create access port. This is a terminal failure')
        while 1:
            pass
    
    # We create a Service Port descriptor. This descriptor is then used for
    # all calls to that specific service port. The service port requires an access port
    sp = kdrive.kdrive_sp_create(ap)
    
    # We check that we were able to allocate a new descriptor
    # This should always happen, unless a bad_alloc exception is internally thrown
    # which means the memory couldn't be allocated.
    if sp == KDRIVE_INVALID_DESCRIPTOR:
        kdrive.kdrive_logger(KDRIVE_LOGGER_FATAL, 'Unable to create service port. This is a terminal failure')
        while 1:
            pass
    
    # Set the device services to connection-oriented or
    # connection-less (depending on the value of connection_oriented)
    kdrive.kdrive_sp_set_co(sp, connection_oriented)
        
    # read property value : serial number
    prop_value_read(sp)
    # write property value : programming mode
    prop_value_write(sp)

    # switch the programming mode on
    switch_prog_mode(sp, 1)
    # read the programming mode
    read_prog_mode(sp)

    # read the individual address of devices which are in programming mode
    ind_addr_prog_mode_read(sp)
    # write the individual address of devices which are in programming mode
    ind_addr_prog_mode_write(sp)

    # switch the programming mode off
    switch_prog_mode(sp, 0)
    # read the programming mode
    read_prog_mode(sp)
    
    # Release the service port
    kdrive.kdrive_sp_release(sp)
    
    # close the access port
    kdrive.kdrive_ap_close(ap)
        
    # releases the access port
    kdrive.kdrive_ap_release(ap)
 

def open_access_port():
    ap = kdrive.kdrive_ap_create()
    if (ap != KDRIVE_INVALID_DESCRIPTOR):
        if (kdrive.kdrive_ap_enum_usb(ap) == 0) or (kdrive.kdrive_ap_open_usb(ap, 0) != KDRIVE_ERROR_NONE):
            kdrive.kdrive_ap_release(ap)
            ap = KDRIVE_INVALID_DESCRIPTOR
    return ap


def prop_value_read(sp):
    """
        Reads the property value from PID_SERIAL_NUMBER
        of device with individual address 'address'
    """
    data = (c_ubyte * SERIAL_NUMBER_LENGTH)()
    data_length = c_uint(SERIAL_NUMBER_LENGTH)

    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Property Value Read')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '===================')

    if (kdrive.kdrive_sp_prop_value_read(sp, address, 0, 11, 1, 1, data, byref(data_length)) == KDRIVE_ERROR_NONE):
        kdrive.kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, 'Read Serial Number: ', data, data_length)


def prop_value_write(sp):
    """
        Writes the property value from PID_PROGMODE
        of device with individual address 'address'
    """
    data = (c_ubyte * 1)(0)

    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Property Value Write')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '====================')

    kdrive.kdrive_sp_prop_value_write(sp, address, 0, 54, 1, 1, data, 1)


def switch_prog_mode(sp, enable):
    """
        Switches the programming mode off or on
        of device with individual address 'address'
    """
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Switch Prog Mode')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '================')

    kdrive.kdrive_sp_switch_prog_mode(sp, address, enable)


def read_prog_mode(sp):
    """
        Reads the programming mode
        of device with individual address 'address'
    """
    enabled = c_uint(0)

    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Read Prog Mode')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '================')

    if (kdrive.kdrive_sp_read_prog_mode(sp, address, byref(enabled)) == KDRIVE_ERROR_NONE):
        kdrive.kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, 'Programming Mode: %s', 'on' if enabled else 'off')


def ind_addr_prog_mode_read(sp):
    """
        Reads the individual addresses of devices
        which are in programming mode.
    """
    data = (c_ushort * MAX_IND_ADDR)()
    data_length = c_uint(MAX_IND_ADDR)

    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Individual Address Prog Mode Read')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '=================================')

    if (kdrive.kdrive_sp_ind_addr_prog_mode_read(sp, 500, data, byref(data_length)) == KDRIVE_ERROR_NONE):
        kdrive.kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, 'Read %d Individual Address(es):', data_length)
        end = data_length.value
        for i in range(end):
            kdrive.kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, '%04X', data[i])


def ind_addr_prog_mode_write(sp):
    """
        Writes the individual address to a device
        which are in programming mode.
    """
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, 'Individual Address Prog Mode Write')
    kdrive.kdrive_logger(KDRIVE_LOGGER_INFORMATION, '==================================')

    kdrive.kdrive_sp_ind_addr_prog_mode_write(sp, 0x05F1)
    ind_addr_prog_mode_read(sp)
    kdrive.kdrive_sp_ind_addr_prog_mode_write(sp, address)
    ind_addr_prog_mode_read(sp)


def on_error_callback(e, user_data):
    len = 1024
    str = create_string_buffer(len)
    kdrive.kdrive_get_error_message(e, str, len)
    print 'kdrive error {0} {1}'.format(hex(e), str.value)


if __name__ == '__main__':
    main()
