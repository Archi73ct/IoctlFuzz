#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctime>
#include <stdint.h>
#define MAX_BUF_SIZE 256

uint8_t MAGIC_FUCK_YOU_NUMBERS_8[] = { 0x00, 0xff, 0x7f, 0x1 };
uint16_t MAGIC_FUCK_YOU_NUMBERS_16[] = { 0x00, 0xffff, 0x7fff, 0x1 };
uint32_t MAGIC_FUCK_YOU_NUMBERS_32[] = { 0x00, 0xffffffff, 0x7fffffff, 0x1 };
uint64_t MAGIC_FUCK_YOU_NUMBERS_64[] = { 0x00, 0xffffffffffffffff, 0x7fffffffffffffff, 0x1 };

void dumpbuf(void* buf, int size)
{
	printf("[");
	for (int i = 0; i < size; i++)
	{
		printf("%02hhx", *((char*)(buf)+i));
	}
	printf("]\n");
}

/* For some reason theres no unistd.h here... */
void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

/*
	This will generate a random buffer, to be stored in outbuf
	it returns the length of the randomly generated buffer.
*/
int genbuf(void* outbuf)
{
	int choice;
	int written = 0;
	int length = (rand() % (MAX_BUF_SIZE));
	choice = (rand() % 8);
	uint64_t idx = 0;
	while (written < (length-12))
	{
		switch (choice)
		{
		case 0:
			// Write magic char
			idx = rand() % sizeof(MAGIC_FUCK_YOU_NUMBERS_8);
			//printf("Writing: %02hhx\n", MAGIC_FUCK_YOU_NUMBERS_8[idx]);
			*((uint8_t*)(outbuf)+written) = MAGIC_FUCK_YOU_NUMBERS_8[idx];
			written++;
			break;
		case 1:
			// Write random char
			idx = rand() % 0xff;
			//printf("Writing: %02hhx\n", idx);
			*((uint8_t*)(outbuf)+written) = (unsigned char)idx;
			written++;
			break;
		case 2:
			// Write magic short
			idx = rand() % (sizeof(MAGIC_FUCK_YOU_NUMBERS_16) / sizeof(uint16_t));
			//printf("Writing11: %04hx\n", MAGIC_FUCK_YOU_NUMBERS_16[idx]);
			*((uint16_t*)(outbuf)+written) = MAGIC_FUCK_YOU_NUMBERS_16[idx];
			written += 2;
			break;
		case 3:
			// Write random short
			idx = rand() % 0xffff;
			//printf("Writing: %04hx\n", idx);
			*((uint16_t*)(outbuf)+written) = (uint16_t)idx;
			written += 2;
			break;
		case 4:
			// Write magic int
			idx = rand() % (sizeof(MAGIC_FUCK_YOU_NUMBERS_32) / sizeof(uint32_t));
			//printf("Writing11: %08x\n", MAGIC_FUCK_YOU_NUMBERS_32[idx]);
			*((uint32_t*)(outbuf)+written) = MAGIC_FUCK_YOU_NUMBERS_32[idx];
			written += 4;
			break;
		case 5:
			// Write random short
			idx = rand() % 0xffffffff;
			//printf("Writing: %08x\n", idx);
			*((uint32_t*)(outbuf)+written) = (uint32_t)idx;
			written += 4;
			break;
		case 6:
			// Write magic long
			idx = rand() % (sizeof(MAGIC_FUCK_YOU_NUMBERS_64) / sizeof(uint64_t));
			*((uint64_t*)(outbuf)+written) = MAGIC_FUCK_YOU_NUMBERS_64[idx];
			written += 8;
			break;
		case 7:
			// Write random long
			idx = ((uint64_t)rand()<<32)+rand();
			*((uint64_t*)(outbuf)+written) = (uint64_t)idx;
			written += 8;
			break;
		}
		choice = (rand() % 8);
	}
	return length;
}

int wmain(int argc, wchar_t* argv[])
{
	int ioctl_code_array[200];
	wchar_t* pt = NULL;
	/*
		We want two arguments, simple...
		1: the name/path of the driver in the global namespace
		2: a comma seperated list of ioctl codes
	*/
	if (argc != 3)
	{
		printf("Wrong args there homie\n");
		printf("Please supply [path_to_driver] [ioctl1],[ioctl2],[ioctl3] <- in hex");
		return -1;
	}

	printf("Got the following two arguments %ls, %ls\n", argv[1], argv[2]);
	wchar_t* filename = argv[1];
	wchar_t* ioctl_list = argv[2];

	wchar_t* iwc = wcstok_s(ioctl_list, L",", &pt);
	int i = 0;
	while (iwc != NULL) {
		swscanf_s(iwc, L"%lX", &ioctl_code_array[i]);
		printf("0x%x\n", ioctl_code_array[i]);
		iwc = wcstok_s(NULL, L",", &pt);
		i++;
	}
	// Seed the random number generator
	srand((unsigned) time(0));

	HANDLE hDriver = CreateFile(filename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		3,
		FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hDriver == INVALID_HANDLE_VALUE)
	{
		printf("Error opening driver handle!\n");
		return -1;
	}

	// Fuzz loop ayay!
	int ioctl_test_code;
	int random_number;

	int tests_executed = 0;

	void* buffer = malloc(MAX_BUF_SIZE);
	void* outbuf = malloc(2048);
	DWORD returned = 0;
	int size;
	while (true) {
		random_number = (rand() % (i));
		ioctl_test_code = ioctl_code_array[random_number];
		memset(buffer, 0, MAX_BUF_SIZE);
		try {
			size = genbuf(buffer);
			bool result = DeviceIoControl(hDriver,
				ioctl_test_code,
				buffer,
				size,
				outbuf,
				2048,
				&returned,
				NULL);
		}
		catch (int e) {
			printf("Error %x occured\n", e);
		}
		if (tests_executed % 1000 == 0) {
			printf(".");
		}
		tests_executed++;
	}

CLEANUP:
	free(buffer);
	free(outbuf);
	CloseHandle(hDriver);
}