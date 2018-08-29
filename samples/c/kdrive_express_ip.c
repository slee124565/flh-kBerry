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

#define TELEGRAM_TIMEOUT	(1000)	/*!< telegram timeout: 1 second */
#define MAX_BUFFER_SIZE		(64)	/*!< max telegram buffer size */
#define ERROR_MESSAGE_LEN	(128)	/*!< kdriveExpress Error Messages */

/*******************************
** Private Functions
********************************/

/*!
	Send a GroupValueWrite and display incoming status telegrams
*/
static void send_group_value_write(int32_t ap, uint16_t address, uint8_t value);

/*!
	Sends a GroupValueRead and waits for the GroupValueResponse
	Displays the address and value
*/
static void read_group_object(int32_t ap, uint16_t address);

/*!
	Telegram Callback Handler
*/
static void on_telegram(const uint8_t* telegram, uint32_t telegram_len, void* user_data);

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
	uint16_t address = 0x901;
	uint32_t index = 0;
	uint32_t key = 0;
	uint8_t value = 127;
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
			Connect the Packet Trace logging mechanism
			to see the Rx and Tx packets
		*/
		kdrive_ap_packet_trace_connect(ap);

		send_group_value_write(ap, address, 1);	/* Write a 1 Bit Boolean: on */
		send_group_value_write(ap, address, 0);	/* Write a 1 Bit Boolean: off */

		/* Read the value of a Communication Object from the bus */
		read_group_object(ap, 0x902);

		/* now we simply go into bus monitor mode, and display received telegrams */
		kdrive_ap_register_telegram_callback(ap, &on_telegram, NULL, &key);

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
	This function sends a GroupValue_Write telegram
	and then wait for rx telegrams. To receive the
	telegrams we use the queue, and we'll keep receiving
	as long as a telegram arrives within the telegram timeout
	period. Once the timeout elapses we disable the queue and
	return. Essentially, this is a demo on how to use the queue
	and doesn't make a lot a sense otherwise. A more likely
	scenario is:

	\code
	kdrive_ap_enable_queue(ap, 1);
	while (!test_for_exit_condition())
	{
		telegram_len = kdrive_ap_receive(ap, telegram_buffer, MAX_BUFFER_SIZE, TELEGRAM_TIMEOUT);
		if (telegram_len)
		{
			... do something with the telegram
		}
	}
	kdrive_ap_enable_queue(ap, 0);
	\endcode
*/
void send_group_value_write(int32_t ap, uint16_t address, uint8_t value)
{
	static uint8_t telegram_buffer[MAX_BUFFER_SIZE];
	uint32_t telegram_len = MAX_BUFFER_SIZE;

	kdrive_ap_group_write(ap, address, &value, 1);

	// enable the receive queue
	kdrive_ap_enable_queue(ap, 1);

	// wait until the bus is idle for 1 second
	while ((telegram_len = kdrive_ap_receive(ap, telegram_buffer, MAX_BUFFER_SIZE, TELEGRAM_TIMEOUT)) != 0)
	{
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "Received telegram from queue :", telegram_buffer, telegram_len);
	}

	// disable the receive queue
	kdrive_ap_enable_queue(ap, 0);
}

/*!
	This function reads the value of a Group Object (associated with a Group Address)
	It uses the kdrive_ap_read_group_object function which handles the
	read state machine (sending GroupValue_Read and waiting for the first GroupValue_Response)
*/
void read_group_object(int32_t ap, uint16_t address)
{
	static uint8_t telegram[MAX_BUFFER_SIZE];
	uint32_t telegram_len = MAX_BUFFER_SIZE;
	static uint8_t data[KDRIVE_MAX_GROUP_VALUE_LEN];
	uint32_t data_len = KDRIVE_MAX_GROUP_VALUE_LEN;

	telegram_len = kdrive_ap_read_group_object(ap, address, telegram, telegram_len, 1000);

	if (telegram_len > 0 &&
	    (kdrive_ap_is_group_response(telegram, telegram_len)) &&
	    (kdrive_ap_get_dest(telegram, telegram_len, &address) == KDRIVE_ERROR_NONE) &&
	    (kdrive_ap_get_group_data(telegram, telegram_len, data, &data_len) == KDRIVE_ERROR_NONE))
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Response: 0x%04x ", address);
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Response Data :", data, data_len);
	}
	else
	{
		printf("A_GroupValue_Response: 0x%04x - timeout", address);
	}
}

/*!
	When a telegram is received we check to see if it is a L_Data.ind group value write
	telegram. If it is, we get the destination address and the datapoint value
*/
void on_telegram(const uint8_t* telegram, uint32_t telegram_len, void* user_data)
{
	static uint8_t data[KDRIVE_MAX_GROUP_VALUE_LEN];
	uint32_t data_len = KDRIVE_MAX_GROUP_VALUE_LEN;
	uint16_t address = 0;
	uint8_t message_code = 0;

	kdrive_ap_get_message_code(telegram, telegram_len, &message_code);

	if ((message_code == KDRIVE_CEMI_L_DATA_IND) &&
	    kdrive_ap_is_group_write(telegram, telegram_len) &&
	    (kdrive_ap_get_dest(telegram, telegram_len, &address) == KDRIVE_ERROR_NONE) &&
	    (kdrive_ap_get_group_data(telegram, telegram_len, data, &data_len) == KDRIVE_ERROR_NONE))
	{
		kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write: 0x%04x ", address);
		kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write Data :", data, data_len);
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

