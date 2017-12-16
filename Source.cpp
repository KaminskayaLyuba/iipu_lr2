#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <WinIoCtl.h>
#include <ntddscsi.h>

using namespace std;

#define bThousand 1024
#define Hundred 100
#define BYTE_SIZE 8

char* busType[] = { "UNKNOWN", "SCSI", "ATAPI", "ATA", "ONE_TREE_NINE_FOUR", "SSA", "FIBRE", "USB", "RAID", "ISCSI", "SAS", "SATA", "SD", "MMC" };
char* driveType[] = { "UNKNOWN", "INVALID", "CARD_READER/FLASH", "HARD", "REMOTE", "CD_ROM", "RAM" };

void getMemoryInfo() {
	string path;
	_ULARGE_INTEGER diskSpace;
	_ULARGE_INTEGER freeSpace;

	diskSpace.QuadPart = 0;
	freeSpace.QuadPart = 0;

	_ULARGE_INTEGER totalDiskSpace;
	_ULARGE_INTEGER totalFreeSpace;
	totalDiskSpace.QuadPart = 0;
	totalFreeSpace.QuadPart = 0;

	unsigned long int logicalDrivesCount = GetLogicalDrives();
	cout.setf(ios::left);
	cout << setw(8) << "Disk"
		<< setw(16) << "Total space[Mb]"
		<< setw(16) << "Free space[Mb]"
		<< setw(16) << "Busy space[%]"
		<< setw(16) << "Driver type"
		<< endl;

	for (char var = 'A'; var < 'Z'; var++) {
		if ((logicalDrivesCount >> var - 65) & 1) {
			path = var;
			path.append(":\\");
			GetDiskFreeSpaceEx(wstring(path.begin(),path.end()).c_str(), 0, &diskSpace, &freeSpace);
			diskSpace.QuadPart = diskSpace.QuadPart / (bThousand * bThousand);
			freeSpace.QuadPart = freeSpace.QuadPart / (bThousand * bThousand);

			cout << setw(8) << var
				<< setw(16) << diskSpace.QuadPart
				<< setw(16) << freeSpace.QuadPart
				<< setw(16) << setprecision(3) << 100.0 - (double)freeSpace.QuadPart / (double)diskSpace.QuadPart * Hundred
				<< setw(16) << driveType[GetDriveType(wstring(path.begin(),path.end()).c_str())]
				<< endl;
			if (GetDriveType(wstring(path.begin(),path.end()).c_str()) == 3) {
				totalDiskSpace.QuadPart += diskSpace.QuadPart;
				totalFreeSpace.QuadPart += freeSpace.QuadPart;
			}
		}
	}

	cout << setw(8) << "HDD"
		<< setw(16) << totalDiskSpace.QuadPart
		<< setw(16) << totalFreeSpace.QuadPart
		<< setw(16) << setprecision(3) << 100.0 - (double)totalFreeSpace.QuadPart / (double)totalDiskSpace.QuadPart * Hundred
		<< driveType[GetDriveType(NULL)]
		<< endl;
}

void getDeviceInfo(HANDLE diskHandle, STORAGE_PROPERTY_QUERY storageProtertyQuery) {
	STORAGE_DEVICE_DESCRIPTOR* deviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)calloc(bThousand, 1);
	deviceDescriptor->Size = bThousand;

	if (!DeviceIoControl(diskHandle, IOCTL_STORAGE_QUERY_PROPERTY, &storageProtertyQuery, sizeof(storageProtertyQuery), deviceDescriptor, bThousand, NULL, 0)) {
		printf("%d", GetLastError());
		CloseHandle(diskHandle);
		exit(-1);
	}

	cout << "Product ID:    " << (char*)(deviceDescriptor)+deviceDescriptor->ProductIdOffset << endl;
	cout << "Version        " << (char*)(deviceDescriptor)+deviceDescriptor->ProductRevisionOffset << endl;
	cout << "Bus type:      " << busType[deviceDescriptor->BusType] << endl;
	cout << "Serial number: " << (char*)(deviceDescriptor)+deviceDescriptor->SerialNumberOffset << endl;
}

void getAtaSupportStandarts(HANDLE diskHandle) {

	UCHAR identifyDataBuffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };

	ATA_PASS_THROUGH_EX &PTE = *(ATA_PASS_THROUGH_EX *)identifyDataBuffer;
	PTE.Length = sizeof(PTE);
	PTE.TimeOutValue = 10;
	PTE.DataTransferLength = 512;
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);
	PTE.AtaFlags = ATA_FLAGS_DATA_IN;

	IDEREGS *ideRegs = (IDEREGS *)PTE.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	if (!DeviceIoControl(diskHandle, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(identifyDataBuffer), &PTE, sizeof(identifyDataBuffer), NULL, NULL)) {
		cout << GetLastError() << std::endl;
		return;
	}

	WORD *data = (WORD *)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));
	short ataSupportByte = data[80];

	WORD MDMA_supports = data[63];

	if (MDMA_supports & 0x4)
	{
		cout << "Multiword DMA supports: 0 1 2" << endl;
	}
	else if (MDMA_supports & 0x2)
	{
		cout << "Multiword DMA supports: 0 1" << endl;
	}
	else if (MDMA_supports & 0x1)
	{
		cout << "Multiword DMA supports: 0" << endl;
	}
	else
	{
		cout << endl;
	}

	WORD UDMA_supports = data[88];

	cout << "Ultra DMA supports: ";
	if (UDMA_supports & 0x40)
	{
		cout << "0 1 2 3 4 5 6" << endl;
	}
	else if (UDMA_supports & 0x20)
	{
		cout << "0 1 2 3 4 5" << endl;
	}
	else if (UDMA_supports & 0x10)
	{
		cout << "0 1 2 3 4" << endl;
	}
	else if (UDMA_supports & 0x8)
	{
		cout << "0 1 2 3" << endl;
	}
	else if (UDMA_supports & 0x4)
	{
		cout << "0 1 2" << endl;
	}
	else if (UDMA_supports & 0x2)
	{
		cout << "0 1" << endl;
	}
	else if (UDMA_supports & 0x1)
	{
		cout << "0" << endl;
	}
	else
	{
		cout << endl;
	}

	WORD PIO_supports = data[64];

	cout << "PIO supports: ";
	if (PIO_supports & 0x2)
	{
		cout << "0 1 2 3 4" << endl;
	}
	else if (PIO_supports & 0x1)
	{
		cout << "0 1 2 3" << endl;
	}
	else
	{
		cout << endl;
	}

	int i = 2 * BYTE_SIZE;
	int bitArray[2 * BYTE_SIZE];
	while (i--) {
		bitArray[i] = ataSupportByte & 32768 ? 1 : 0;
		ataSupportByte = ataSupportByte << 1;
	}

	cout << "ATA Support:   ";
	for (int i = 8; i >= 4; i--) {
		if (bitArray[i] == 1) {
			cout << "ATA " << i;
			if (i != 4) {
				cout << ", ";
			}
		}
	}
	cout << endl;
}

int main() {
	STORAGE_PROPERTY_QUERY storageProtertyQuery;
	storageProtertyQuery.QueryType = PropertyStandardQuery;
	storageProtertyQuery.PropertyId = StorageDeviceProperty;

	HANDLE diskHandle = CreateFile(L"//./PhysicalDrive0", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (diskHandle == INVALID_HANDLE_VALUE) {
		cout << GetLastError();
		return -1;
	}

	getDeviceInfo(diskHandle, storageProtertyQuery);
	getAtaSupportStandarts(diskHandle);
	getMemoryInfo();

	CloseHandle(diskHandle);
	return 0;
}