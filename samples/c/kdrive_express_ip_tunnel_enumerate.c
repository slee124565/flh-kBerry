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

#define ERROR_MESSAGE_LEN	(128)	/*!< kdriveExpress Error Messages */
#define MAX_ITEMS (50) /*!< The maximum number of items (struct with tunneling devices) we expect to read at one time */

/*******************************
** Private Functions
********************************/

static void enumerate_tunneling_devices(int32_t ap);

/*!
	Called when an error occurs
*/
static void error_callback(error_t e, void* user_data);

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	int32_t ap = 0;

	/*
		Configure the logging level and console logger
	*/
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
	ap = kdrive_ap_create();

	/*
		We check that we were able to allocate a new descriptor
		This should always happen, unless a bad_alloc exception is internally thrown
		which means the memory couldn't be allocated.
	*/
	if (ap == KDRIVE_INVALID_DESCRIPTOR)
	{
		printf("Unable to create access port. This is a terminal failure\n");
		while (1)
		{
			;
		}
	}

	enumerate_tunneling_devices(ap);

	/* releases the access port */
	kdrive_ap_release(ap);

	return 0;
}

/*******************************
** Private Functions
********************************/

/*!
	Enumerats the tunneling ip interfaces
*/
void enumerate_tunneling_devices(int32_t ap)
{
	ip_tunn_dev_t items[MAX_ITEMS];
	uint32_t items_length = MAX_ITEMS;
	uint32_t index = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Enumerating KNX IP Tunneling Interfaces");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "========================================");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");

	if (kdrive_ap_enum_ip_tunn(ap, items, &items_length) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Found %d device(s)", items_length);

		for (index = 0; index < items_length; ++index)
		{
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "%d) Name: %s", index + 1, items[index].dev_name);
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "%s on %s", items[index].ip_address, items[index].iface_address);
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Individual Address: %04X", items[index].ind_addr);
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Programming Mode: %s", (items[index].prog_mode_enabled ? "on" : "off"));
		}

		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");
	}
}

/*!
	Called when a kdrive error exception is raised.
	The handling in the error callback is typically
	application specific. And here we simply show
	the error message.
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
