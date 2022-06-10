/* Copyright 2022 Xiaomi Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Semihost functionality for redirecting stdin/stdout/stderr I/O to the GNU console. */
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <core/cmd_file.h>
#include <core/fileio.h>
#include <nuttx/fs/hostfs.h>

static int convert_flag(int flags)
{
    int newflags = 0;

    if (flags & O_RDWR)
        newflags |= GDB_O_RDWR;
    else if (flags & O_WRONLY)
        newflags |= GDB_O_WRONLY;

    if (flags & O_APPEND)
        newflags |= GDB_O_APPEND;

    if (flags & O_CREAT)
        newflags |= GDB_O_CREAT;

    if (flags & O_TRUNC)
        newflags |= GDB_O_TRUNC;

    if (flags & O_EXCL)
        newflags |= GDB_O_EXCL;

    return newflags;
}

static int convert_result(int ret)
{
    if (ret == 0)
        return -EINTR;

    ret = GetSemihostReturnCode();
    if (ret >= 0)
        return ret;

    return -GetSemihostErrno();
}

int host_open(const char* pathname, int flags, int mode)
{
    OpenParameters parameters;

    parameters.filenameAddress = (uintptr_t)pathname;
    parameters.filenameLength = strlen(pathname) + 1;
    parameters.flags = convert_flag(flags);
    parameters.mode = mode;

    return convert_result(IssueGdbFileOpenRequest(&parameters));
}

int host_close(int fd)
{
    return convert_result(IssueGdbFileCloseRequest(fd));
}

ssize_t host_read(int fd, void* buf, size_t count)
{
    TransferParameters parameters;

    parameters.fileDescriptor = fd;
    parameters.bufferAddress =(uintptrt_t)buf;
    parameters.bufferSize = count;

    return convert_result(IssueGdbFileReadRequest(&parameters));
}

ssize_t host_write(int fd, const void* buf, size_t count)
{
    TransferParameters parameters;

    parameters.fileDescriptor = fd;
    parameters.bufferAddress =(uintptrt_t)buf;
    parameters.bufferSize = count;

    return convert_result(IssueGdbFileWriteRequest(&parameters));
}

off_t host_lseek(int fd, off_t offset, int whence)
{
    SeekParameters parameters;

    parameters.fileDescriptor = fd;
    parameters.offset = offset;
    parameters.whence = whence;

    return convert_result(IssueGdbFileSeekRequest(&parameters));
}

int host_ioctl(int fd, int request, unsigned long arg)
{
  return -ENOSYS;
}

void host_sync(int fd)
{
}

int host_dup(int fd)
{
  return -ENOSYS;
}

static void convert_stat(struct stat* buf, const GdbStats* stats)
{
    memset(buf, sizeof(*buf));

    buf->st_dev         = be32toh(stats->device);
    buf->st_ino         = be32toh(stats->inode);
    buf->st_mode        = be32toh(stats->mode);
    buf->st_nlink       = be32toh(stats->numberOfLinks);
    buf->st_uid         = be32toh(stats->userId);
    buf->st_gid         = be32toh(stats->groupId);
    buf->st_rdev        = be32toh(stats->deviceType);
    buf->st_size        = be64toh(stats->totalSizeUpperWord | (uint64_t)
                                  stats->totalSizeLowerWord << 32);
    buf->st_blksize     = be64toh(stats->blockSizeUpperWord | (uint64_t)
                                  stats->blockSizeLowerWord << 32);;
    buf->st_blocks      = be64toh(stats->blockCountUpperWord | (uint64_t)
                                  stats->blockCountLowerWord << 32);;
    buf->st_atim.tv_sec = be32toh(stats->lastAccessTime);
    buf->st_mtim.tv_sec = be32toh(stats->lastModifiedTime);
    buf->st_ctim.tv_sec = be32toh(stats->lastChangeTime);
}

int host_fstat(int fd, struct stat* buf)
{
    GdbStats stats;
    int ret;

    ret = IssueGdbFileFStatRequest(fd, (uintptr_t)&stats);
    if (ret)
        convert_stat(buf, &stats);

    return convert_result(ret);
}

int host_fchstat(int fd, const struct stat* buf, int flags)
{
    return -ENOSYS;
}

int host_ftruncate(int fd, off_t length)
{
    return -ENOSYS;
}

void *host_opendir(const char* name)
{
    return NULL;
}

int host_readdir(void* dirp, struct dirent* entry)
{
    return -ENOSYS;
}

void host_rewinddir(void* dirp)
{
}

int host_closedir(void* dirp)
{
    return -ENOSYS;
}

int host_statfs(const char* path, struct statfs* buf)
{
    return 0;
}

int host_unlink(const char* pathname)
{
    RemoveParameters parameters;

    parameters.filenameAddress = (uintptr_t)pathname;
    parameters.filenameLength = strlen(pathname) + 1;

    return convert_result(IssueGdbFileUnlinkRequest(&parameters));
}

int host_mkdir(const char* pathname, mode_t mode)
{
    return -ENOSYS;
}

int host_rmdir(const char* pathname)
{
    return convert_result(host_unlink(pathname));
}

int host_rename(const char* oldpath, const char* newpath)
{
    RenameParameters parameters;

    parameters.origFilenameAddress = (uintptr_t)oldpath;
    parameters.origFilenameLength = strlen(oldpath) + 1;
    parameters.newFilenameAddress = (uintptr_t)newpath;
    parameters.newFilenameLength = strlen(newpath) + 1;

    return convert_result(IssueGdbFileRenameRequest(&parameters));
}

int host_stat(const char* path, struct stat* buf)
{
    StatParameters parameters;
    GdbStats stats;
    int ret;

    parameters.filenameAddress = (uintptr_t)path;
    parameters.filenameLength = strlen(path) + 1;
    parameters.fileStatBuffer = &stats;

    ret = mriIssueGdbFileStatRequest(&parameters);
    if (ret)
        convert_stat(buf, &stats);

    return convert_result(ret);
}

int host_chstat(const char* path, const struct stat* buf, int flags)
{
    return -ENOSYS;
}
