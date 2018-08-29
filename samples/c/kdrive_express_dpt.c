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

#define ERROR_MESSAGE_LEN	(128)						/*!< kdriveExpress Error Messages */

#define ADDR_DPT_1			(0x0901)					/*!< Group Address of DTP-1 */
#define ADDR_DPT_2			(0x090A)					/*!< Group Address of DTP-2 */
#define ADDR_DPT_3			((ADDR_DPT_2) + 1)			/*!< Group Address of DTP-3 */
#define ADDR_DPT_4			((ADDR_DPT_3) + 1)			/*!< Group Address of DTP-4 */
#define ADDR_DPT_5			((ADDR_DPT_4) + 1)			/*!< Group Address of DTP-5 */
#define ADDR_DPT_6			((ADDR_DPT_5) + 1)			/*!< Group Address of DTP-6 */
#define ADDR_DPT_7			((ADDR_DPT_6) + 1)			/*!< Group Address of DTP-7 */
#define ADDR_DPT_8			((ADDR_DPT_7) + 1)			/*!< Group Address of DTP-8 */
#define ADDR_DPT_9			((ADDR_DPT_8) + 1)			/*!< Group Address of DTP-9 */
#define	ADDR_DPT_10_LOCAL	((ADDR_DPT_9) + 1)			/*!< Group Address of DTP-10 Local Time */
#define	ADDR_DPT_10_UTC		((ADDR_DPT_10_LOCAL) + 1)	/*!< Group Address of DTP-10 UTC Time */
#define	ADDR_DPT_10			((ADDR_DPT_10_UTC) + 1)		/*!< Group Address of DTP-10 */
#define	ADDR_DPT_11_LOCAL	((ADDR_DPT_10) + 1)			/*!< Group Address of DTP-11 Local Date */
#define	ADDR_DPT_11_UTC		((ADDR_DPT_11_LOCAL) + 1)	/*!< Group Address of DTP-11 UTC Date */
#define	ADDR_DPT_11			((ADDR_DPT_11_UTC) + 1)		/*!< Group Address of DTP-11 */
#define	ADDR_DPT_12			((ADDR_DPT_11) + 1)			/*!< Group Address of DTP-12 */
#define	ADDR_DPT_13			((ADDR_DPT_12) + 1)			/*!< Group Address of DTP-13 */
#define ADDR_DPT_14			((ADDR_DPT_13) + 1)			/*!< Group Address of DTP-14 */
#define ADDR_DPT_15			((ADDR_DPT_14) + 1)			/*!< Group Address of DTP-15 */
#define ADDR_DPT_16			((ADDR_DPT_15) + 1)			/*!< Group Address of DTP-16 */

/*******************************
** Private Functions
********************************/

/*!
	Sends group value write telegrams with various
	datapoint value formats
*/
static void send_telegrams(int32_t ap);

/*!
	Telegram Callback Handler
*/
static void on_telegram_callback(const uint8_t* telegram, uint32_t telegram_len, void* user_data);

/*!
	Called when an error occurs
*/
static void error_callback(error_t e, void* user_data);

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	uint8_t value = 1; /*!< The value we send on the bus with the Group Value Write command: on */
	uint32_t key = 0; /*!< used by the telegram callback mechanism. uniquely identifies a callback */
	uint32_t iface_count = 0; /*!< holds the number of USB interfaces found by USB enumeration */
	int32_t ap = 0; /*!< the Access Port descriptor */

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

	iface_count = kdrive_ap_enum_usb(ap);
	kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "Found %d KNX USB Interfaces", iface_count);

	/* If we found at least 1 interface we simply open the first one (i.e. index 0) */
	if ((iface_count > 0) && (kdrive_ap_open_usb(ap, 0) == KDRIVE_ERROR_NONE))
	{
		/* register to receive telegrams */
		kdrive_ap_register_telegram_callback(ap, &on_telegram_callback, NULL, &key);

		/* send group value write telegrams with various datapoint formats */
		send_telegrams(ap);

		/* go into bus monitor mode */
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
	Sends Group Value Write telegrams
	for Datapoint Types 1 through 16
*/
void send_telegrams(int32_t ap)
{
	uint8_t buffer[KDRIVE_MAX_GROUP_VALUE_LEN];
	uint32_t length;

	/* DPT-1 (1 bit) */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt1(buffer, &length, 1);
	kdrive_ap_group_write(ap, ADDR_DPT_1, buffer, length);

	/* DPT-2: 1 bit controlled */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt2(buffer, &length, 1, 1);
	kdrive_ap_group_write(ap, ADDR_DPT_2, buffer, length);

	/* DPT-3: 3 bit controlled */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt3(buffer, &length, 1, 0x05);
	kdrive_ap_group_write(ap, ADDR_DPT_3, buffer, length);

	/* DPT-4: Character */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt4(buffer, &length, 'A');
	kdrive_ap_group_write(ap, ADDR_DPT_4, buffer, length);

	/* DPT-5: 8 bit unsigned value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt5(buffer, &length, 0x23);
	kdrive_ap_group_write(ap, ADDR_DPT_5, buffer, length);

	/* DPT-6: 8 bit signed value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt6(buffer, &length, -5);
	kdrive_ap_group_write(ap, ADDR_DPT_6, buffer, length);

	/* DPT-7: 2 byte unsigned value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt7(buffer, &length, 0xAFFE);
	kdrive_ap_group_write(ap, ADDR_DPT_7, buffer, length);

	/* DPT-8: 2 byte signed value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt8(buffer, &length, -1024);
	kdrive_ap_group_write(ap, ADDR_DPT_8, buffer, length);

	/* DPT-9: 2 byte float value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt9(buffer, &length, 12.25f);
	kdrive_ap_group_write(ap, ADDR_DPT_9, buffer, length);

	/* DPT-10: Local Time */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt10_local(buffer, &length);
	kdrive_ap_group_write(ap, ADDR_DPT_10_LOCAL, buffer, length);

	/* DPT-10: UTC Time */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt10_utc(buffer, &length);
	kdrive_ap_group_write(ap, ADDR_DPT_10_UTC, buffer, length);

	/* DPT-10: Time */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt10(buffer, &length, 1, 11, 11, 11);
	kdrive_ap_group_write(ap, ADDR_DPT_10, buffer, length);

	/* DPT-11: Local Date */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt11_local(buffer, &length);
	kdrive_ap_group_write(ap, ADDR_DPT_11_LOCAL, buffer, length);

	/* DPT-11: UTC Date */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt11_utc(buffer, &length);
	kdrive_ap_group_write(ap, ADDR_DPT_11_UTC, buffer, length);

	/* DPT-11: Date */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt11(buffer, &length, 2012, 03, 12);
	kdrive_ap_group_write(ap, ADDR_DPT_11, buffer, length);

	/* DPT-12: 4 byte unsigned value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt12(buffer, &length, 0xDEADBEEF);
	kdrive_ap_group_write(ap, ADDR_DPT_12, buffer, length);

	/* DPT-13: 4 byte signed value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt13(buffer, &length, -30000);
	kdrive_ap_group_write(ap, ADDR_DPT_13, buffer, length);

	/* DPT-14: 4 byte float value */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt14(buffer, &length, 2025.12345f);
	kdrive_ap_group_write(ap, ADDR_DPT_14, buffer, length);

	/* DPT-15: Entrance access */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt15(buffer, &length, 1234, 0, 1, 1, 0, 10);
	kdrive_ap_group_write(ap, ADDR_DPT_15, buffer, length);

	/* DPT-16: Character string, 14 bytes */
	length = KDRIVE_MAX_GROUP_VALUE_LEN;
	kdrive_dpt_encode_dpt16(buffer, &length, "Weinzierl Eng ");
	kdrive_ap_group_write(ap, ADDR_DPT_16, buffer, length);
}

/*!
	When a telegram is received we check to see if it is a group value write
	telegram. If it is, we log the datapoint value.
	\note The L_Data.con telegrams will in this sample also logged.
	If you want only see the L_Data.ind then you should check the message code.
	\see kdrive_ap_get_message_code
*/
void on_telegram_callback(const uint8_t* telegram, uint32_t telegram_len, void* user_data)
{
	static uint8_t data[KDRIVE_MAX_GROUP_VALUE_LEN];
	static uint8_t sn[KDRIVE_SN_LEN];
	uint32_t data_len = KDRIVE_MAX_GROUP_VALUE_LEN;
	uint16_t address = 0;
	uint8_t level = KDRIVE_LOGGER_INFORMATION;

	if ((kdrive_ap_is_group_write(telegram, telegram_len)) &&
	    (kdrive_ap_get_dest(telegram, telegram_len, &address) == KDRIVE_ERROR_NONE) &&
	    (kdrive_ap_get_group_data(telegram, telegram_len, data, &data_len) == KDRIVE_ERROR_NONE))
	{
		switch (address)
		{
			case ADDR_DPT_1:
			{
				bool_t value = 0;
				kdrive_dpt_decode_dpt1(data, data_len, &value);
				kdrive_logger_ex(level, "[1 Bit] %d", value ? 1 : 0);
			}
			break;

			case ADDR_DPT_2:
			{
				bool_t control = 0;
				bool_t value = 0;
				kdrive_dpt_decode_dpt2(data, data_len, &control, &value);
				kdrive_logger_ex(level, "[1 Bit controlled] %d %d", control, value);
			}
			break;

			case ADDR_DPT_3:
			{
				bool_t control = 0;
				uint8_t value = 0;
				kdrive_dpt_decode_dpt3(data, data_len, &control, &value);
				kdrive_logger_ex(level, "[3 Bit controlled] %d %d", control, value);
			}
			break;

			case ADDR_DPT_4:
			{
				uint8_t character = 0;
				kdrive_dpt_decode_dpt4(data, data_len, &character);
				kdrive_logger_ex(level, "[Character] %d", character);
			}
			break;

			case ADDR_DPT_5:
			{
				uint8_t value = 0;
				kdrive_dpt_decode_dpt5(data, data_len, &value);
				kdrive_logger_ex(level, "[8 bit unsigned] 0x%02x", value);
			}
			break;

			case ADDR_DPT_6:
			{
				int8_t value = 0;
				kdrive_dpt_decode_dpt6(data, data_len, &value);
				kdrive_logger_ex(level, "[8 bit signed] %d", value);
			}
			break;

			case ADDR_DPT_7:
			{
				uint16_t value = 0;
				kdrive_dpt_decode_dpt7(data, data_len, &value);
				kdrive_logger_ex(level, "[2 byte unsigned] 0x%04x", value);
			}
			break;

			case ADDR_DPT_8:
			{
				int16_t value = 0;
				kdrive_dpt_decode_dpt8(data, data_len, &value);
				kdrive_logger_ex(level, "[2 byte signed] %d", value);
			}
			break;

			case ADDR_DPT_9:
			{
				float32_t value = 0;
				kdrive_dpt_decode_dpt9(data, data_len, &value);
				kdrive_logger_ex(level, "[2 byte float] %f", value);
			}
			break;

			case ADDR_DPT_10_LOCAL:
			case ADDR_DPT_10_UTC:
			case ADDR_DPT_10:
			{
				int32_t day = 0;
				int32_t hour = 0;
				int32_t minute = 0;
				int32_t second = 0;
				kdrive_dpt_decode_dpt10(data, data_len, &day, &hour, &minute, &second);
				kdrive_logger_ex(level, "[time] %d %d %d %d", day, hour, minute, second);
			}
			break;

			case ADDR_DPT_11_LOCAL:
			case ADDR_DPT_11_UTC:
			case ADDR_DPT_11:
			{
				int32_t year = 0;
				int32_t month = 0;
				int32_t day = 0;
				kdrive_dpt_decode_dpt11(data, data_len, &year, &month, &day);
				kdrive_logger_ex(level, "[date] %d %d %d", year, month, day);
			}
			break;

			case ADDR_DPT_12:
			{
				uint32_t value = 0;
				kdrive_dpt_decode_dpt12(data, data_len, &value);
				kdrive_logger_ex(level, "[4 byte unsigned] 0x%08x", value);
			}
			break;

			case ADDR_DPT_13:
			{
				uint32_t value = 0;
				kdrive_dpt_decode_dpt13(data, data_len, &value);
				kdrive_logger_ex(level, "[4 byte signed] %d", value);
			}
			break;

			case ADDR_DPT_14:
			{
				float32_t value = 0;
				kdrive_dpt_decode_dpt14(data, data_len, &value);
				kdrive_logger_ex(level, "[4 byte float] %f", value);
			}
			break;

			case ADDR_DPT_15:
			{
				int32_t accessCode = 0;
				bool_t error = 0;
				bool_t permission = 0;
				bool_t direction = 0;
				bool_t encrypted = 0;
				int32_t index = 0;
				kdrive_dpt_decode_dpt15(data, data_len, &accessCode, &error, &permission, &direction, &encrypted, &index);
				kdrive_logger_ex(level, "[entrance access] %d %d %d %d %d",
				                 accessCode, error, permission, direction, encrypted, index);
			}
			break;

			case ADDR_DPT_16:
			{
				/* kdrive_dpt_decode_dpt16 is not null terminated, so we null terminate */
				char value[KDRIVE_DPT16_LENGTH + 1];
				kdrive_dpt_decode_dpt16(data, data_len, value);
				value[KDRIVE_DPT16_LENGTH] = '\0';
				kdrive_logger_ex(level, "[character string] %s", value);
			}
			break;

			default:
				kdrive_logger_ex(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write: 0x%04x ", address);
				kdrive_logger_dump(KDRIVE_LOGGER_INFORMATION, "A_GroupValue_Write Data :", data, data_len);
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
