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

// Replace this with your serial device name
// e.g. on linux /dev/ttyS0
#define SERIAL_DEVICE ("COM1")

/*******************************
** Main
********************************/

int main(int argc, char* argv[])
{
	uint16_t address = 0x901; /*!< The KNX Group Address (destination address) we use to send with */
	uint8_t value = 0; /*!< The value we send on the bus with the Group Value Write command */
	uint32_t key = 0; /*! used by the telegram callback mechanism. uniquely identifies a callback */
	int32_t ap = 0; /*! the Access Port descriptor */

	/*
		Configure the logging level and console logger
	*/
	kdrive_logger_set_level(KDRIVE_LOGGER_INFORMATION);
	kdrive_logger_console();

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

	if (kdrive_ap_open_tiny_serial(ap, SERIAL_DEVICE) == KDRIVE_ERROR_NONE)
	{
		/*
			Connect the Packet Trace logging mechanism
			to see the Rx and Tx packets
		*/
		kdrive_ap_packet_trace_connect(ap);

		/* send a 1-Bit boolean GroupValueWrite telegram: on */
		value = 1;
		kdrive_ap_group_write(ap, address, &value, 1);

		/* send a 1-Bit boolean GroupValueWrite telegram: off */
		value = 0;
		kdrive_ap_group_write(ap, address, &value, 1);

		/* close the access port */
		kdrive_ap_close(ap);
	}

	/* releases the access port */
	kdrive_ap_release(ap);

	return 0;
}
