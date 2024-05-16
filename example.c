/*
 * Example client using COBS encoding and decoding
 * 
 * Copyright 1997-1999, 2004, 2007, Stuart Cheshire <http://www.stuartcheshire.org/>
 * 
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See <http://www.gnu.org/copyleft/lesser.html>
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * For API documentation see COBSEncoder.h
 */

#include <stdio.h>
#include "COBSEncoder.h"

static char *hexprint(char *buffer, const unsigned char *data, unsigned int length)
	{
	unsigned int i;
	buffer[0] = 0;
	for (i=0; i+1<length; i++) sprintf(buffer+i*3, "%02X ", data[i]);
	if (length > 0)            sprintf(buffer+i*3, "%02X",  data[i]);
	
	return(buffer);
	}

// This example encodes in one shot, and decodes in one shot
static void one_shot_example(const unsigned char *data, int length)
	{
	unsigned char buffer[256];
	unsigned char enc[256];
	unsigned char dec[256];
	unsigned char *ep = enc;
	const unsigned char *sp = enc;
	unsigned char *dp = dec;
	
	// 1. Encode data and add framing marker
	printf("Initial data     : %s\n", hexprint(buffer, data, length));
	COBSEncode(data, length, &ep, NULL);
	*ep++ = 0;
	
	// 2. Data is now ready to ship over serial byte-stream
	// 'ep' points to one byte past the last byte of encoded data;
	// i.e. 'ep-enc' gives the number of bytes to be shipped over
	// the serial byte-stream
	printf("Framed message   : %s\n", hexprint(buffer, enc, ep-enc));
	
	// 3. Receiver decodes data like this:
	COBSDecode(&sp, ep-enc, &dp, sizeof(dec), NULL);
	printf("Decoded data     : %s\n", hexprint(buffer, dec, dp-dec));
	printf("\n");
	}

// This example encodes incrementally, giving the encoder just one byte at a
// time. The partial state at each step of the encoding process is displayed.
static void pipeline_encode(const unsigned char *data, int length)
	{
	unsigned char buffer[256];
	unsigned char enc[256];
	unsigned char *ep = enc;
	unsigned char *encoding_state = NULL;
	int i;

	// 1. Encode data, feeding just one byte at a time into COBSEncode.
	// Each single byte fed in may advance 'ep' by zero, one, or two bytes.
	printf("Initial data     : %s\n", hexprint(buffer, data, length));
	for (i=0; i<length; i++)
		{
		COBSEncode(&data[i], 1, &ep, &encoding_state);
		printf("Partial          : %s\n", hexprint(buffer, enc, ep-enc));
		if (encoding_state > enc)	// Do we have some data ready to send?
			{
			// The bytes from 'enc' to 'encoding_state' can be sent;
			// the bytes from 'encoding_state' to 'ep' need to be preserved.
			int send = encoding_state - enc;
			int save = ep - encoding_state;
			printf("Transmit %3d     : %s\n", send, hexprint(buffer, enc, send));

			// Now slide the remaining bytes to the start of our buffer
			memmove(enc, enc+send, save);
			// And slide pointers back by the same amount
			encoding_state -= send;
			ep             -= send;
			printf("Partial          : %s\n", hexprint(buffer, enc, ep-enc));
			}
		}

	// 2. Add framing marker, and display final block of sent data
	*ep++ = 0;
	printf("Transmit %3d     : %s\n", ep-enc, hexprint(buffer, enc, ep-enc));

	printf("\n");
	}

// This example shows incremental decoding, simulating bytes being
// received one at a time from a serial port or similar.
// The partial state at each step of the decoding process is displayed.
static void pipeline_decode1(const unsigned char *data, int length)
	{
	unsigned char buffer[256];
	unsigned char enc[256];
	unsigned char *ep = enc;
	const unsigned char *sp = enc;
	unsigned char dec[256];
	unsigned char *dp = dec;
	unsigned short decoding_state = 0;

	// 1. Create some data for this example decoder to work on
	printf("Initial data     : %s\n", hexprint(buffer, data, length));
	COBSEncode(data, length, &ep, NULL);
	*ep++ = 0;
	
	// 2. Data is now ready to ship over serial byte-stream
	// 'ep' points to one byte past the last byte of encoded data;
	// i.e. 'ep-enc' gives the number of bytes to be shipped over
	// the serial byte-stream
	printf("Framed message   : %s\n", hexprint(buffer, enc, ep-enc));

	// 3. Receiver decodes data.
	// Here we simulate the bytes arriving on the serial port,
	// and just hand COBSDecode one byte at a time to work on.
	// When COBSDecode returns 1 (true) that means we've got a whole message.
	// Each single byte fed into COBSDecode may advance 'dp' by zero, one,
	// two or even 30 bytes (in the case of a single code byte encoding a
	// run of 30 zeroes)
	while (1)
		{
		unsigned char dec[256];
		unsigned char *dp = dec;
		unsigned char *lim = dec + sizeof(dec);
		unsigned char val = *sp;
		int eom = COBSDecode(&sp, 1, &dp, lim-dp, &decoding_state);
		printf("Read %02X; Output  : %s\n", val, hexprint(buffer, dec, dp-dec));
		if (eom) break;
		}
	printf("\n");
	}

// This example shows incremental decoding, simulating output bytes being
// extracted and handed to some other entity one at a time.
// The partial state at each step of the decoding process is displayed.
static void pipeline_decode2(const unsigned char *data, int length)
	{
	unsigned char buffer[256];
	unsigned char enc[256];
	unsigned char *ep = enc;
	const unsigned char *sp = enc;
	unsigned char dec[256];
	unsigned char *dp = dec;
	unsigned short decoding_state = 0;

	// 1. Create some data for this example decoder to work on
	printf("Initial data     : %s\n", hexprint(buffer, data, length));
	COBSEncode(data, length, &ep, NULL);
	*ep++ = 0;
	
	// 2. Data is now ready to ship over serial byte-stream
	// 'ep' points to one byte past the last byte of encoded data;
	// i.e. 'ep-enc' gives the number of bytes to be shipped over
	// the serial byte-stream
	printf("Framed message   : %s\n", hexprint(buffer, enc, ep-enc));

	// 3. Receiver decodes data.
	// Here we simulate feeding some entity that wants just one byte at a time,
	// so we only allow COBSDecode one byte of buffer space to write into.
	// When COBSDecode returns 1 (true) that means we've got a whole message.
	// Each single byte extracted into COBSDecode may advance 'sp' by
	// zero, one, or two bytes, but never more than two bytes.
	while (1)
		{
		unsigned char dec[1];
		unsigned char *dp = dec;
		const unsigned char *ip = sp;
		int eom = COBSDecode(&sp, ep-sp, &dp, 1, &decoding_state);
		printf("Read %5s Output: %02X\n", hexprint(buffer, ip, sp-ip), dec[0]);
		if (eom) break;
		}
	printf("\n");
	}

int main(int argc, char **argv)
	{
	unsigned char *ep, *dp;
	const unsigned char *sp;
	const unsigned char x0[ 5] = "ABCDE";			// Example with no zeroes
	const unsigned char x1[ 6] = "ABCDE\0";			// Example with trailing zero
	const unsigned char x2[ 6] = "ABC\0DE";			// Example with interior zero
	const unsigned char x3[ 6] = "\0ABCDE";			// Example with leading zero
	const unsigned char x4[10] = "ABC\0\0\0\0\0DE";	// Example with run of zeroes

	printf("\n");
	printf("One-shot encoding and decoding.\n\n");
	one_shot_example(x0, sizeof(x0));
	one_shot_example(x1, sizeof(x1));
	one_shot_example(x2, sizeof(x2));
	one_shot_example(x3, sizeof(x3));
	one_shot_example(x4, sizeof(x4));

	printf("Pipeline encoding, one byte at a time, sending whenever possible.\n\n");
	pipeline_encode(x0, sizeof(x0));
	pipeline_encode(x1, sizeof(x1));
	pipeline_encode(x2, sizeof(x2));
	pipeline_encode(x3, sizeof(x3));
	pipeline_encode(x4, sizeof(x4));

	printf("Pipeline decoding, reading just one source byte at a time.\n\n");
	pipeline_decode1(x0, sizeof(x0));
	pipeline_decode1(x1, sizeof(x1));
	pipeline_decode1(x2, sizeof(x2));
	pipeline_decode1(x3, sizeof(x3));
	pipeline_decode1(x4, sizeof(x4));

	printf("Pipeline decoding, extracting just one output byte at a time.\n\n");
	pipeline_decode2(x0, sizeof(x0));
	pipeline_decode2(x1, sizeof(x1));
	pipeline_decode2(x2, sizeof(x2));
	pipeline_decode2(x3, sizeof(x3));
	pipeline_decode2(x4, sizeof(x4));

	return(0);
	}
