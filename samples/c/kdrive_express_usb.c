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
#define MAX_USB_INTERFACES	(10)	/*!< Max usb interfaces */

/*******************************
** Private Functions
********************************/

/*!
	Telegram Callback Handler
*/
static void on_telegram_callback(const uint8_t* telegram, uint32_t telegram_len, void* user_data);

/*!
	Called when an error occurs
*/
static void error_callback(error_t e, void* user_data);

/*!
	Called when an event occurs
*/
static void event_callback(int32_t ap, uint32_t e, void* user_data);

/*******************************
** Private Variables
********************************/

/*
	Is the usb interface a RF-Interface.

	For Twisted Pair and Powerline:
		Set to 0

	For RF:
		Set to 1
*/
static bool_t is_rf = 0;

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	uint16_t address = 0x901; /*!< The KNX Group Address (destination address) we use to send with */
	uint8_t value = 1; /*!< The value we send on the bus with the Group Value Write command: on */
	uint32_t key = 0; /*! used by the telegram callback mechanism. uniquely identifies a callback */
	int32_t ap = 0; /*! the Access Port descriptor */
	usb_dev_t iface_items[MAX_USB_INTERFACES]; /*!< holds the usb interface info found by USB enumeration */
	uint32_t iface_items_length = MAX_USB_INTERFACES; /*!< holds the number of items in iface_items */
	uint32_t index = 0; /*!< usd by loop index */
	usb_dev_t* iface; /*!< usb interface info usd by loop */

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
		kdrive_logger(KDRIVE_LOGGER_FATAL, "Unable to create access port. This is a terminal failure");
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

	kdrive_ap_enum_usb_ex(iface_items, &iface_items_length);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Found %d KNX USB Interfaces", iface_items_length);

	for (index = 0; index < iface_items_length; ++index)
	{
		iface = &iface_items[index];
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Interface %d)", index);
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- indiviual address 0x%04X", iface->ind_addr);
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- media tytes 0x%04X", iface->media_tytes);
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- internal usb index %d", iface->internal_usb_index);
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- usb vendor id %d", iface->usb_vendor_id);
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "- usb product id %d", iface->usb_product_id);
		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "");
	}

	/* If we found at least 1 interface we simply open the first one (i.e. index 0) */
	if ((iface_items_length > 0) && (kdrive_ap_open_usb(ap, 0) == KDRIVE_ERROR_NONE))
	{
		/*
			Connect the Packet Trace logging mechanism
			to see the Rx and Tx packets
		*/
		kdrive_ap_packet_trace_connect(ap);

		/* send a 1-Bit boolean GroupValueWrite telegram: on */
		kdrive_ap_group_write(ap, address, &value, 1);

		/* now we simply go into bus monitor mode, and display received telegrams */
		kdrive_ap_register_telegram_callback(ap, &on_telegram_callback, NULL, &key);

		kdrive_logger(KDRIVE_LOGGER_INFORMATION, "Entering BusMonitor Mode");
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
	When a telegram is received we check to see if it is a L_Data.ind group value write
	telegram. If it is, we get the destination address and the datapoint value
	For KNX-RF Telegrams we additionally display the Serial Number extracted
	from Additional Info.
	Note: You should only need kdrive_ap_get_serial_number if you are working with RF.
*/
void on_telegram_callback(const uint8_t* telegram, uint32_t telegram_len, void* user_data)
{
	static uint8_t data[KDRIVE_MAX_GROUP_VALUE_LEN];
	static uint8_t sn[KDRIVE_SN_LEN];
	uint32_t data_len = KDRIVE_MAX_GROUP_VALUE_LEN;
	uint16_t address = 0;
	uint8_t message_code = 0;

	kdrive_ap_get_message_code(telegram, telegram_len, &message_code);

	if (message_code == KDRIVE_CEMI_L_DATA_IND)
	{
		if (kdrive_ap_is_group_write(telegram, telegram_len) &&
		    (kdrive_ap_get_dest(telegram, telegram_len, &address) == KDRIVE_ERROR_NONE) &&
		    (kdrive_ap_get_group_data(telegram, telegram_len, data, &data_len) == KDRIVE_ERROR_NONE))
		{
			kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write: 0x%04x ", address);
			kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write Data :", data, data_len);
		}

		if (is_rf)
		{
			if (kdrive_ap_get_serial_number(telegram, telegram_len, sn) == KDRIVE_ERROR_NONE)
			{
				kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Serial Number :", sn, KDRIVE_SN_LEN);
			}
		}
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

