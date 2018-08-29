//
// Copyright (c) 2002-2016 WEINZIERL ENGINEERING GmbH
// All rights reserved.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY,
// WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
//

#include <stdio.h>
#include <stdlib.h>
#include <kdrive_express.h>

#define SERIAL_NUMBER_LENGTH (6) /*!< The length of a KNX Serial Number */
#define ERROR_MESSAGE_LEN (128) /*!< kdriveExpress Error Messages buffer length */
#define MAX_IND_ADDR (5) /*!< The maximum number of individual addresses we expect to read at one time */
#define MAX_ITEMS (5) /*!< The maximum number of items (struct with individual address, domain address and serial number) we expect to read at one time */

/*******************************
** Private Functions
********************************/

static int32_t open_access_port();
static void set_rf_domain_address(int32_t ap);
static void prop_value_read(int32_t sp);
static void prop_value_write(int32_t sp);
static void switch_prog_mode(int32_t sp, bool_t enable);
static void read_prog_mode(int32_t sp);
static void memory_read(int32_t sp);
static void memory_write(int32_t sp);
static void ind_addr_prog_mode_read(int32_t sp);
static void ind_addr_prog_mode_write(int32_t sp);
static void ind_addr_sn_read(int32_t sp);
static void ind_addr_sn_write(int32_t sp);
static void domain_addr_prog_mode_read(int32_t sp);
static void domain_addr_prog_mode_write(int32_t sp);
static void domain_addr_sn_read(int32_t sp);
static void domain_addr_sn_write(int32_t sp);
static void error_callback(error_t e, void* user_data);

/*******************************
** Private Variables
********************************/

/*
	For Twisted Pair:
		Set to 0

	For RF:
		Set to 1
*/
static bool_t is_rf = 0;

/*
	For Twisted Pair:
		Set to 1 to use connection-oriented
		Set to 0 for connection-less

	For RF:
		Set to 0 for connection-less
*/
static bool_t connection_oriented = 1;

/*!
	The address of the device that we connect
	to for the device services (Property Value Read etc)
*/
static uint16_t address = 0x5102;

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	/* create a access and service port */
	int32_t ap = KDRIVE_INVALID_DESCRIPTOR;
	int32_t sp = KDRIVE_INVALID_DESCRIPTOR;

	/* Configure the logging level and console logger */
	kdrive_logger_set_level(KDRIVE_LOGGER_INFORMATION);
	kdrive_logger_console();

	/*
		We register an error callback as a convenience logger function to
		print out the error message when an error occurs.
	*/
	kdrive_register_error_callback(&error_callback, NULL);

	/*
		We create a Access Port descriptor. This descriptor is then used for
		all calls to that specific access port.
	*/
	ap = open_access_port();

	/*
		We check that we were able to allocate a new descriptor
		This should always happen, unless a bad_alloc exception is internally thrown
		which means the memory couldn't be allocated.
	*/
	if (ap == KDRIVE_INVALID_DESCRIPTOR)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Unable to create access port. This is a terminal failure");
		while (1)
		{
			;
		}
	}

	/*
		KNX USB Local Device Management
		Set the RF Domain Address
		This will only work for interfaces that support RF
		(i.e. KNX TP USB interfaces will raise an error here, we ignore it)
	*/
	if (is_rf)
	{
		set_rf_domain_address(ap);
	}

	/*
		We create a Service Port descriptor. This descriptor is then used for
		all calls to that specific service port. The service port requires
		an access port
	*/

	sp = kdrive_sp_create(ap);

	/*
		We check that we were able to allocate a new descriptor
		This should always happen, unless a bad_alloc exception is internally thrown
		which means the memory couldn't be allocated.
	*/
	if (sp == KDRIVE_INVALID_DESCRIPTOR)
	{
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Unable to create service port. This is a terminal failure");
		while (1)
		{
			;
		}
	}

	/*
		Set the device services to connection-oriented or
		connection-less (depending on the value of connection_oriented)
	*/
	kdrive_sp_set_co(sp, connection_oriented);

	prop_value_read(sp); /* read property value : serial number */
	prop_value_write(sp); /* write property value : programming mode */

	if (is_rf)
	{
		domain_addr_sn_read(sp); /* read the domain address via serial number */
		domain_addr_sn_write(sp); /* write the domain address via serial number */
	}

	ind_addr_sn_read(sp); /* read the individual address via serial number */
	ind_addr_sn_write(sp); /* write the individual address via serial number */

	switch_prog_mode(sp, 1); /* switch the programming mode on */
	read_prog_mode(sp); /* read the programming mode */

	if (is_rf)
	{
		domain_addr_prog_mode_read(sp); /* read the domain address of devices which are in programming mode */
		domain_addr_prog_mode_write(sp); /* write the domain address of devices which are in programming mode */
	}

	ind_addr_prog_mode_read(sp); /* read the individual address of devices which are in programming mode */
	ind_addr_prog_mode_write(sp); /* write the individual address of devices which are in programming mode */

	switch_prog_mode(sp, 0); /* switch the programming mode off */
	read_prog_mode(sp); /* read the programming mode */

	memory_read(sp); /* read memory: programming mode */
	memory_write(sp); /* write memory: programming mode */

	/* Release the service port */
	kdrive_sp_release(sp);
	sp = KDRIVE_INVALID_DESCRIPTOR;

	/* Close the access port */
	kdrive_ap_close(ap);
	kdrive_ap_release(ap);

	return 0;
}

/*******************************
** Private Functions
********************************/

static int32_t open_access_port()
{
	int32_t ap = kdrive_ap_create();

	if (ap != KDRIVE_INVALID_DESCRIPTOR)
	{
		if ((kdrive_ap_enum_usb(ap) == 0) ||
		    (kdrive_ap_open_usb(ap, 0) != KDRIVE_ERROR_NONE))
		{
			kdrive_ap_release(ap);
			ap = KDRIVE_INVALID_DESCRIPTOR;
		}
	}

	return ap;
}

void set_rf_domain_address(int32_t ap)
{
	uint8_t da[KDRIVE_DA_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	kdrive_ap_set_rf_domain_addr(ap, da);
}

/*!
	Reads the property value from PID_SERIAL_NUMBER
	of device with individual address "address"
*/
void prop_value_read(int32_t sp)
{
	uint8_t data[SERIAL_NUMBER_LENGTH];
	uint32_t data_length = SERIAL_NUMBER_LENGTH;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Property Value Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "===================");

	if (kdrive_sp_prop_value_read(sp, address, 0, 11, 1, 1, data, &data_length) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Read Serial Number: ", data, data_length);
	}
}

/*!
	Writes the property value from PID_PROGMODE
	of device with individual address "address"
*/
void prop_value_write(int32_t sp)
{
	uint8_t data = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Property Value Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "====================");

	kdrive_sp_prop_value_write(sp, address, 0, 54, 1, 1, &data, 1);
}

/*!
	Switches the programming mode off or on
	of device with individual address "address"
*/
void switch_prog_mode(int32_t sp, bool_t enable)
{
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Switch Prog Mode");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "================");

	kdrive_sp_switch_prog_mode(sp, address, enable);
}

/*!
	Reads the programming mode
	of device with individual address "address"
*/
void read_prog_mode(int32_t sp)
{
	bool_t enabled = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Read Prog Mode");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "================");

	if (kdrive_sp_read_prog_mode(sp, address, &enabled) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Programming Mode: %s", enabled ? "on" : "off");
	}
}

/*!
	Reads memory on 0x0060 (= programming mode)
	of device with individual address "address"
*/
void memory_read(int32_t sp)
{
	uint8_t data;
	uint32_t data_length = 1;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Memory Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "===================");

	if (kdrive_sp_memory_read(sp, address, 0x0060, 1, &data, &data_length) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Read Prog mode: ", &data, data_length);
	}
}

/*!
	Writes memory on 0x0060 (= programming mode)
	of device with individual address "address" to 0 (= off)
*/
void memory_write(int32_t sp)
{
	uint8_t data = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Memory Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "====================");

	kdrive_sp_memory_write(sp, address, 0x0060, &data, 1);
}

/*!
	Reads the individual addresses of devices
	which are in programming mode.
*/
void ind_addr_prog_mode_read(int32_t sp)
{
	uint32_t index = 0;
	uint16_t data[MAX_IND_ADDR];
	uint32_t data_length = MAX_IND_ADDR;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Individual Address Prog Mode Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "=================================");

	if (kdrive_sp_ind_addr_prog_mode_read(sp, 500, data, &data_length) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Read %d Individual Address(es):", data_length);
		for (index = 0; index < data_length; ++index)
		{
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "%04X", data[index]);
		}
	}
}

/*!
	Writes the individual address to a device
	which are in programming mode.
*/
void ind_addr_prog_mode_write(int32_t sp)
{
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Individual Address Prog Mode Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "==================================");

	kdrive_sp_ind_addr_prog_mode_write(sp, 0x05F1);
	ind_addr_prog_mode_read(sp);
	kdrive_sp_ind_addr_prog_mode_write(sp, address);
	ind_addr_prog_mode_read(sp);
}

/*!
	Reads the individual address to a device
	with serial number 00 C5 00 00 00 FA
*/
void ind_addr_sn_read(int32_t sp)
{
	uint8_t sn[KDRIVE_SN_LEN] = { 0x00, 0xC5, 0x00, 0x00, 0x00, 0xFA };
	uint16_t ind_addr = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Individual Address Serial Number Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "=====================================");

	if (kdrive_sp_ind_addr_sn_read(sp, sn, &ind_addr) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Read Individual Address: %d", ind_addr);
	}
}

/*!
	Writes the individual address to a device
	with serial number 00 C5 00 00 00 FA
*/
void ind_addr_sn_write(int32_t sp)
{
	uint8_t sn[KDRIVE_SN_LEN] = { 0x00, 0xC5, 0x00, 0x00, 0x00, 0xFA };

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Individual Address Serial Number Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "======================================");

	kdrive_sp_ind_addr_sn_write(sp, sn, 0x05F1);
	ind_addr_sn_read(sp);
	kdrive_sp_ind_addr_sn_write(sp, sn, address);
	ind_addr_sn_read(sp);
}

/*!
	Reads the domain addresses of devices
	which are in programming mode.
*/
void domain_addr_prog_mode_read(int32_t sp)
{
	uint32_t index = 0;
	domain_addr_prog_mode_read_t items[MAX_ITEMS];
	uint32_t items_length = MAX_ITEMS;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Domain Address Prog Mode Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "=============================");

	if (kdrive_sp_domain_addr_prog_mode_read(sp, 500, items, &items_length) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Read %d item(s):", items_length);
		for (index = 0; index < items_length; ++index)
		{
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Individual Address : %04X", items[index].ind_addr);
			kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Serial Number :", items[index].serial_number, KDRIVE_SN_LEN);
			kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Domain Address :", items[index].domain_address, KDRIVE_DA_LEN);
		}
	}
}

/*!
	Writes the domain address to a device
	which are in programming mode.
*/
void domain_addr_prog_mode_write(int32_t sp)
{
	uint8_t da1[KDRIVE_DA_LEN] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	uint8_t da2[KDRIVE_DA_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Domain Address Prog Mode Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "==============================");

	kdrive_sp_domain_addr_prog_mode_write(sp, da1);
	domain_addr_prog_mode_read(sp);

	kdrive_sp_domain_addr_prog_mode_write(sp, da2);
	domain_addr_prog_mode_read(sp);
}

/*!
	Reads the domain address to a device
	with serial number 00 C5 00 00 00 FA
*/
void domain_addr_sn_read(int32_t sp)
{
	uint8_t sn[KDRIVE_SN_LEN] = { 0x00, 0xC5, 0x00, 0x00, 0x00, 0xFA };
	uint16_t ind_addr = 0;
	uint8_t da[KDRIVE_DA_LEN] = { 0 };

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Domain Address Serial Number Read");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "=================================");

	if (kdrive_sp_domain_addr_sn_read(sp, sn, &ind_addr, da) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Individual Address : %04X", ind_addr);
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Domain Address :", da, KDRIVE_DA_LEN);
	}
}

/*!
	Writes the domain address to a device
	with serial number 00 C5 00 00 00 FA
*/
void domain_addr_sn_write(int32_t sp)
{
	uint8_t sn[KDRIVE_SN_LEN] = { 0x00, 0xC5, 0x00, 0x00, 0x00, 0xFA };
	uint8_t da1[KDRIVE_DA_LEN] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	uint8_t da2[KDRIVE_DA_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Domain Address Serial Number Write");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "==================================");

	kdrive_sp_domain_addr_sn_write(sp, sn, da1);
	domain_addr_sn_read(sp);

	kdrive_sp_domain_addr_sn_write(sp, sn, da2);
	domain_addr_sn_read(sp);
}

/*!
	Called when an error occurs
*/
void error_callback(error_t e, void* user_data)
{
	if (e != KDRIVE_TIMEOUT_ERROR)
	{
		static char error_message[ERROR_MESSAGE_LEN];
		kdrive_get_error_message(e, error_message, ERROR_MESSAGE_LEN);
		kdrive_logger_ex(KDRIVE_LOGGER_ERROR, "kdrive error: %s", error_message);
	}
}
