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
    ret = DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
                          &dwBytesReturned, NULL);
    if (!ret) {
        cout << "execute FSCTL_LOCK_VOLUME error ,error code:" << GetLastError()
             << endl;
    } else {
        cout << "execute FSCTL_LOCK_VOLUME succeed" << endl;
    }
    ret = WriteFile(handle, buffer, size, &readsize, &over);
    if (!ret) {
        cout << "write sector error ,error code:" << GetLastError() << endl;
    } else {
        cout << "write sector succeed" << endl;
    }
    DeviceIoControl(handle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
                    &dwBytesReturned, NULL);
    delete[] buffer;
    //注意这里需要自己释放内存
    CloseHandle(handle);
    return 0;
}

int main(int argc, _TCHAR* argv[]) {
    unsigned char* a;
    unsigned char in[1024];
    for (int j = 0; j < 1024; j++) {
        in[j] = 'B';
    }
    in[510] = 0x55;
    in[511] = 0xAA;
    in[1022] = 0x55;
    in[1023] = 0xAA;
    /**
     * 返回值：GetLogicalDrives函数返回一个DWORD类型的值，第一位表示所对应的驱动器是否存在。一般情况下DWORD的数据长度是32位，在这个DWORD中，每一位对应了一个逻辑驱动器是否存在。第二位如果是“1”则表示驱动器“B:”存在，第四位如果是“1”则表示驱动器“D:”存在，以此类推
     */
    DWORD logic = GetLogicalDrives();
    char m_strDrives[200];
    DWORD r = GetLogicalDriveStrings(200, m_strDrives);
    for(int i = 0; i < r; i++)
        printf("%c", m_strDrives[i]);
    UINT DType = GetDriveType("G:\\");
    if (DType == DRIVE_FIXED) {
        cout << "硬盘" << endl;
    } else if (DType == DRIVE_CDROM) {
        cout << "光驱" << endl;
    } else if (DType == DRIVE_REMOVABLE) {
        cout << "可移动硬盘" << endl;
    } else if (DType == DRIVE_REMOTE) {
        cout << "网络磁盘" << endl;
    } else if (DType == DRIVE_RAMDISK) {
        cout << "虚拟RAM磁盘" << endl;
    } else if (DType == DRIVE_UNKNOWN) {
        cout << "未知设备" << endl;
    }
//    得出磁盘的可用空间
    DWORD dwTotalClusters; //总的簇
    DWORD dwFreeClusters; //可用的簇
    DWORD dwSectPerClust; //每个簇有多少个扇区
    DWORD dwBytesPerSect; //每个扇区有多少个字节
    BOOL bResult = GetDiskFreeSpace(TEXT("C:"), &dwSectPerClust,
                                    &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters);
    if (bResult) {
        cout << "使用GetDiskFreeSpace函数获取磁盘空间信息" << endl;
        cout << "总簇数量: " << dwTotalClusters << endl;
        cout << "可用的簇: " << dwFreeClusters << endl;
        cout << "每个簇有多少个扇区: " << dwSectPerClust << endl;
        cout << "每个扇区有多少个字节: " << dwBytesPerSect << endl;
        cout << "磁盘总容量: "
             << dwTotalClusters * (DWORD64) dwSectPerClust
             * (DWORD64) dwBytesPerSect << endl;
        cout << "磁盘空闲容量: "
             << dwFreeClusters * (DWORD64) dwSectPerClust
             * (DWORD64) dwBytesPerSect << endl;
    }
    DWORD64 qwFreeBytes, qwFreeBytesToCaller, qwTotalBytes;
    bResult = GetDiskFreeSpaceEx(TEXT("C:"),
                                 (PULARGE_INTEGER) &qwFreeBytesToCaller,
                                 (PULARGE_INTEGER) &qwTotalBytes, (PULARGE_INTEGER) &qwFreeBytes);
    if (bResult) {
        cout << "使用GetDiskFreeSpaceEx函数获取磁盘空间信息" << endl;
        cout << "磁盘总容量: " << qwTotalBytes << endl;
        cout << "可用的磁盘空闲容量: " << qwFreeBytes << endl;
        cout << "磁盘空闲容量: " << qwFreeBytesToCaller << endl;
    }
//    WriteDisk(in, 0, 512 * 2);
    DWORD len = ReadDisk(a, 0, 512 * 2);
    if (len) {
        printf(" offset   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F     0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
        for(int i = 0; i < len; i += 16) {
            printf("%08X", i);
            for(int j = i; j < i + 16; j++) {
                printf(" %02X", a[j]);
            }
            printf("    ");
            for(int k = i; k < i + 16; k++) {
                if(0x0A == a[k]) printf("  ");
                else printf(" %c", a[k]);
            }
            printf("\n");
        }
    }
    getchar();
    return 0;
}
