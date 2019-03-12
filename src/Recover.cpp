#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <iostream>
#include <winbase.h>
#include <windows.h>
#include <string.h>
#include <winioctl.h>
using namespace std;

/**参数：输出的字符串指针,开始位置,长度
 * 返回值：读取的大小
 */
DWORD ReadDisk(unsigned char* &out, DWORD start, DWORD size) {
	OVERLAPPED over = { 0 };
	over.Offset = start;
	HANDLE handle = CreateFile(TEXT("\\\\.\\PHYSICALDRIVE1"), GENERIC_READ,
	FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("read,get handle error\n");
		return 0;
	}
	unsigned char* buffer = new unsigned char[size + 1];
	DWORD readsize;
	SetFilePointer(handle, 0, 0, FILE_BEGIN);
	if (ReadFile(handle, buffer, size, &readsize, &over) == 0) {
		printf("read sector error\n");
		CloseHandle(handle);
		return 0;
	}
	buffer[size] = 0;
	out = buffer;
	delete[] buffer;
	//注意这里需要自己释放内存
	CloseHandle(handle);
	return size;
}
/**
 * 参数：字符串数组，开始位置，写数据大小
 * 返回：
 * 说明：win7及以上，磁盘有磁盘号的不能写物理磁盘，需要写逻辑磁盘
 *     写扇区前需要锁定卷或卸载卷
 */
DWORD WriteDisk(unsigned char* out, DWORD start, DWORD size) {
	OVERLAPPED over = { 0 };
	over.Offset = start;
	HANDLE handle = CreateFile(TEXT("\\\\.\\PHYSICALDRIVE1"), GENERIC_WRITE,
	FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
//	HANDLE handle = CreateFileA("\\\\.\\F:",GENERIC_WRITE | GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
//	HANDLE handle = CreateFileA("\\\\.\\PHYSICALDRIVE1",GENERIC_WRITE | GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("write get handle error\n");
		return 0;
	}
	unsigned char* buffer = new unsigned char[size];
	memcpy(buffer, out, size);
	DWORD readsize;
	DWORD dwBytesReturned;
	SetFilePointer(handle, 512 * 0, 0, FILE_BEGIN);
	DISK_GEOMETRY DiskGeom1;
	DWORD return_bytes, ret;
//	DeviceIoControl(handle,FSCTL_DISMOUNT_VOLUME,NULL,0,NULL,0,&dwBytesReturned,NULL);
	for (int i = 0; i < 10; ++i) {
		DWORD writablelen;
		BOOL writable = DeviceIoControl(handle, IOCTL_DISK_IS_WRITABLE, NULL, 0,
		NULL, 0, &writablelen, NULL);
		if (!writable)
			cout << "Disk is not writable,error code:" << GetLastError();
		else {
			cout << i << " Disk is writable" << endl;
			break;
		}
		Sleep(1000);
	}
//	CREATE_DISK disk;
//	disk.PartitionStyle=PARTITION_STYLE_MBR;
//	disk.Mbr.Signature=1;
//
//	DWORD returnedLength;
//	ret=DeviceIoControl(handle, IOCTL_DISK_CREATE_DISK, &disk, sizeof(CREATE_DISK), NULL, 0, &returnedLength, NULL);
//	if (!ret)
//	{
////	retcode = 1;
//		cout << "Initalize MBR format failed,error code:" << GetLastError();
//	}else{
//		cout << "Initalize MBR format succeed" <<endl;
//	}
//	CloseHandle(handle);
//	if (!DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
//			&DiskGeom1, sizeof(DISK_GEOMETRY), &return_bytes, NULL)) {
//		ret = GetLastError();
//		CloseHandle(handle);
//		printf("controller ret = %d\n", ret);
//		return ret;
//	}
	ret = DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,&dwBytesReturned, NULL);
	if (!ret) {
		cout << "execute FSCTL_LOCK_VOLUME error ,error code:" << GetLastError()<< endl;
	} else {
		cout << "execute FSCTL_LOCK_VOLUME succeed" << endl;
	}
	ret = WriteFile(handle, buffer, size, &readsize, &over);
	if (!ret) {
		cout << "write sector error ,error code:" << GetLastError() << endl;
	} else {
		cout << "write sector succeed" << endl;
	}
	DeviceIoControl(handle,FSCTL_UNLOCK_VOLUME,NULL,0,NULL,0,&dwBytesReturned,NULL);
	delete[] buffer;
	//注意这里需要自己释放内存
	CloseHandle(handle);
	return 0;
}

int main(int argc, _TCHAR* argv[]) {
	unsigned char* a;
	unsigned char in[1024];
	for (int j = 0; j < 1024; j++) {
		in[j] = 'a';
	}
	in[510] = 0x55;
	in[511] = 0xAA;
	in[1022] = 0x55;
	in[1023] = 0xAA;
	WriteDisk(in, 0, 512*2);
//	ShellExecute(0, "runas", "Recover.exe", NULL, NULL, SW_SHOWNORMAL);
	DWORD len = ReadDisk(a, 0, 512 * 2);
	if (len) {
		cout << len << endl;
		for (int i = 0; i < 512; i++) {
			printf("%02X ", a[i]);
		}
		printf("\n");
		for (int i = 512; i < len; i++) {
			printf("%02X ", a[i]);
		}
	}
	getchar();
	return 0;
}
