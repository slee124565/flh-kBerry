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

/*******************************
** Private Functions
********************************/

static void read_additional_individual_addresses(int32_t ap);
static uint16_t read_tunn_ind_addr(int32_t ap);
static void write_tunn_ind_addr(int32_t ap, uint16_t address);

/*!
	Called when an error occurs
*/
static void error_callback(error_t e, void* user_data);

/*!
	Called when an event occurs
*/
static void event_callback(int32_t ap, uint32_t e, void* user_data);

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	uint16_t tunnel_address = 0xFF12;
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

	/*
		We register an event callback to notify of the Access Port Events
		For example: KDRIVE_EVENT_TERMINATED
	*/
	kdrive_set_event_callback(ap, &event_callback, NULL);

	/*
		Open a Tunneling connection with a specific IP Interface,
		you will probably have to change the IP address
	*/
	if (kdrive_ap_open_ip(ap, "192.168.1.45") == KDRIVE_ERROR_NONE)
	{
		/*
			Read all additional individual addresses
		*/
		read_additional_individual_addresses(ap);

		/*
			Read the tunnel individual address
		*/
		tunnel_address = read_tunn_ind_addr(ap);

		/*
			Write the tunnel individual address
		*/
		write_tunn_ind_addr(ap, 0xFF12);
		read_tunn_ind_addr(ap);
		write_tunn_ind_addr(ap, tunnel_address);
		read_tunn_ind_addr(ap);

		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Press [Enter] to exit the application ...");
		getchar();

		/* close the access port */
		kdrive_ap_close(ap);
	}

	/* releases the access port */
	kdrive_ap_release(ap);

	return 0;
}

/*******************************
** Private Functions
********************************/

/*!
	This function read the additional individual addresses of the tunneling interface
*/
void read_additional_individual_addresses(int32_t ap)
{
	uint32_t index = 0;
	uint16_t addresses[10];
	uint32_t addresses_len = 10;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Read all additional individual addresses");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "========================================");

	if (kdrive_ap_get_additional_ind_addr(ap, addresses, &addresses_len) == KDRIVE_ERROR_NONE)
	{
		for (index = 0; index < addresses_len; ++index)
		{
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- 0x%04X", addresses[index]);
		}
	}

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");
}

/*!
	This function read the used individual address for the tunneling connection
	of the tunneling interface
*/
uint16_t read_tunn_ind_addr(int32_t ap)
{
	uint16_t address = 0;

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Read the tunnel individual address");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "==================================");

	if (kdrive_ap_get_tunnel_ind_addr(ap, &address) == KDRIVE_ERROR_NONE)
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Tunnel address 0x%04X", address);
	}

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");

	return address;
}

/*!
	This function writes the used individual address for the tunneling connection
	of the tunneling interface
*/
void write_tunn_ind_addr(int32_t ap, uint16_t address)
{
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Write the tunnel individual address");
	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "===================================");

	kdrive_ap_set_tunnel_ind_addr(ap, address);

	kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");
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

/*!
	The event callback is called when an Access Port event is raised
*/
void event_callback(int32_t ap, uint32_t e, void* user_data)
{
	switch (e)
	{
		case KDRIVE_EVENT_ERROR:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Error");
			break;

		case KDRIVE_EVENT_OPENING:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Opening");
			break;

		case KDRIVE_EVENT_OPENED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Opened");
			break;

		case KDRIVE_EVENT_CLOSED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Closed");
			break;

		case KDRIVE_EVENT_CLOSING:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Closing");
			break;

		case KDRIVE_EVENT_TERMINATED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Access Port Terminated");
			break;

		case KDRIVE_EVENT_KNX_BUS_CONNECTED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "KNX Bus Connected");
			break;

		case KDRIVE_EVENT_KNX_BUS_DISCONNECTED:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "KNX Bus Disconnected");
			break;

		case KDRIVE_EVENT_LOCAL_DEVICE_RESET:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Local Device Reset");
			break;

		case KDRIVE_EVENT_TELEGRAM_INDICATION:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Indication");
			break;

		case KDRIVE_EVENT_TELEGRAM_CONFIRM:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Confirm");
			break;

		case KDRIVE_EVENT_TELEGRAM_CONFIRM_TIMEOUT:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Telegram Confirm Timeout");
			break;

		case KDRIVE_EVENT_INTERNAL_01:
			break;

		default:
			kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Unknown kdrive event");
			break;
	}
}

