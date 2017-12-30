#include "fullfat.h"
#include "board.h"
#include "card.h"
#include "messaging.h"

uint32_t Timer_Get(uint32_t offset);

static FF_IOMAN* pIoman = NULL;  // file system handle
static uint8_t fatBuf[FS_FATBUF_SIZE];
static char error_buf[256];

static FF_T_SINT32 partitionBlockSize;
static FF_T_UINT32 volumeSize, freeSize;

#define TIME(x) \
    { \
        uint32_t t0 = Timer_Get(0); \
        x; \
        FF_GetErrDescription(error, error_buf, sizeof(error_buf)); \
        MSG_debug(0, __FILE__, __LINE__, "\t(%d ms) %s", (Timer_Get(0) - t0) >> 20, #x, error_buf); \
    }
#define CHECK(x) \
    { \
        uint32_t t0 = Timer_Get(0); \
        FF_ERROR err = x; \
        t0 = (Timer_Get(0) - t0) >> 20; \
        FF_GetErrDescription(err, error_buf, sizeof(error_buf)); \
        MSG_debug(0, __FILE__, __LINE__, "\t(%d ms) %s => %s", t0, #x, error_buf); \
    }
#define SETUP_TEST \
    FF_ERROR error = FF_ERR_NONE; \
    (void)error; \
    MSG_debug(0, __FILE__, __LINE__, "%s start", __FUNCTION__);
#define TEAR_DOWN \
    MSG_debug(0, __FILE__, __LINE__, "%s done.", __FUNCTION__);
#define EXPECT(a,b) \
    { \
        if ((a) == (b)) \
            MSG_debug(0, __FILE__, __LINE__, "\t%s == %s OK", #a, #b); \
        else \
            MSG_debug(0, __FILE__, __LINE__, "FAIL:\t%s == %s FAILED!", #a, #b); \
    }

static void TEST_Setup()
{
    SETUP_TEST;

    uint8_t cardType = Card_Init();
    DEBUG(1, "\tcardType : %d", cardType);

    TIME(pIoman = FF_CreateIOMAN(fatBuf, FS_FATBUF_SIZE, 512, &error));
    CHECK(FF_RegisterBlkDevice(pIoman, 512, (FF_WRITE_BLOCKS) Card_WriteM, (FF_READ_BLOCKS) Card_ReadM, NULL));
    CHECK(FF_MountPartition(pIoman, 0));

    TEAR_DOWN;
}

static void TEST_SetInfoNoCheck()
{
    SETUP_TEST;

    TIME(partitionBlockSize = FF_GetPartitionBlockSize(pIoman));
    TIME(volumeSize = FF_GetVolumeSize(pIoman));
    TIME(freeSize = FF_GetFreeSize(pIoman, &error));
    CHECK(FF_IncreaseFreeClusters(pIoman, 0));

    DEBUG(1, "\tpartitionBlockSize : %d", partitionBlockSize);
    DEBUG(1, "\tvolumeSize : %d", volumeSize);
    DEBUG(1, "\tfreeSize : %d", freeSize);

    TEAR_DOWN;
}

static void TEST_Teardown()
{
    SETUP_TEST;

    FF_T_BOOL isMounted;
    TIME(isMounted = FF_Mounted(pIoman));
    EXPECT(isMounted, 1);
    CHECK(FF_UnmountPartition(pIoman));
    CHECK(FF_UnregisterBlkDevice(pIoman));
    CHECK(FF_DestroyIOMAN(pIoman));

    TEAR_DOWN;
}

static void TEST_GetInfo()
{
    SETUP_TEST;

    FF_T_SINT32 blkSize;
    FF_T_UINT32 volSize, avail;
    TIME(blkSize = FF_GetPartitionBlockSize(pIoman));
    TIME(volSize = FF_GetVolumeSize(pIoman));
    TIME(avail = FF_GetFreeSize(pIoman, &error));
    CHECK(FF_IncreaseFreeClusters(pIoman, 0));

    DEBUG(1, "\tpartitionBlockSize : %d", blkSize);
    DEBUG(1, "\tvolumeSize : %d", volSize);
    DEBUG(1, "\tfreeSize : %d", avail);

    TEAR_DOWN;
}

static void TEST_VerifyInfo()
{
    SETUP_TEST;

    FF_T_SINT32 blkSize;
    FF_T_UINT32 volSize, avail;
    TIME(blkSize = FF_GetPartitionBlockSize(pIoman));
    TIME(volSize = FF_GetVolumeSize(pIoman));
    TIME(avail = FF_GetFreeSize(pIoman, &error));
    CHECK(FF_IncreaseFreeClusters(pIoman, 0));

    EXPECT(blkSize, partitionBlockSize);
    EXPECT(volSize, volumeSize);
    EXPECT(avail, freeSize);

    TEAR_DOWN;
}

static const char* dirs[] = {
    //	"/fullfat-test",
    "/fullfat-test/dummy",
    //	"/fullfat-test/aaaa",
    "/fullfat-test/aaaa/bbbb",
    //	"/fullfat-test",
    //	"/fullfat-test/foo",
    //	"/fullfat-test/foo/bar",
    //	"/fullfat-test/foo/bar/foo",
    //	"/fullfat-test/foo/bar/foo/bar",
    //	"/fullfat-test/foo/bar/foo/bar/foo",
    "/fullfat-test/foo/bar/foo/bar/foo/bar",
    //	"/fullfat-test/bar",
    //	"/fullfat-test/bar/foo",
    "/fullfat-test/bar/foo/bar"
};


static void TEST_MkDir()
{
    SETUP_TEST;

    for (int i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
        DEBUG(1, "\tdirs[i] : %s", dirs[i]);
        CHECK(FF_MkDirTree(pIoman, dirs[i]));
        EXPECT(FF_isDirEmpty(pIoman, dirs[i]), FF_TRUE);
    }

    TEAR_DOWN;
}

static __attribute__((unused)) void TEST_RmDir()
{
    SETUP_TEST;

    for (int i = sizeof(dirs) / sizeof(dirs[0]) - 1; i >= 0; --i) {
        DEBUG(1, "\tdirs[i] : %s", dirs[i]);
        CHECK(FF_RmDir(pIoman, dirs[i]));
    }

    TEAR_DOWN;
}


static void TEST_RmDirTree()
{
    SETUP_TEST;

    CHECK(FF_RmDirTree(pIoman, "/fullfat-test"));

    TEAR_DOWN;
}
static const char* files[] = {
    "/fullfat-test/dummy/file.txt",
    "/fullfat-test/aaaa/file.txt",
    "/fullfat-test/aaaa/bbbb/file.txt",
    "/fullfat-test/file.txt",
    "/fullfat-test/foo/file.txt",
    "/fullfat-test/foo/bar/file.txt",
    "/fullfat-test/foo/bar/foo/file.txt",
    "/fullfat-test/foo/bar/foo/bar/file.txt",
    "/fullfat-test/foo/bar/foo/bar/foo/file.txt",
    "/fullfat-test/foo/bar/foo/bar/foo/bar/file.txt",
    "/fullfat-test/bar/file.txt",
    "/fullfat-test/bar/foo/file.txt",
    "/fullfat-test/bar/foo/bar/file.txt"
};

extern const uint8_t loader[];

static void TEST_OpenWriteClose()
{
    SETUP_TEST;

    for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        char filename[256];
        strcpy(filename, files[i]);
        DEBUG(1, "\tfiles[i] : %s", filename);
        FF_FILE* pFile = NULL;
        TIME(pFile = FF_Open(pIoman, filename, FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, &error));
        FF_T_SINT32 written = 0;
        size_t total_len = 0;

        for (int block = 0; block < 1; ++block) {
            uint8_t* p = (uint8_t*)0x00100000;
            TIME(written = FF_Write(pFile, 1, 256 * 1024, p));
            total_len += written;
        }

        EXPECT(total_len, 1024*256*1);

        CHECK(FF_Close(pFile));

        for (size_t c = strlen(filename); c != 0; --c) {
            if (filename[c] == '/') {
                filename[c] = '\0';
                break;
            }
        }
        EXPECT(FF_isDirEmpty(pIoman, filename), FF_FALSE);
    }

    TEAR_DOWN;
}

static uint8_t buffer[1024];
static void TEST_TimeOpenReadClose()
{
    SETUP_TEST;

    for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        const char* filename = files[i];
        DEBUG(1, "\tfiles[i] : %s", filename);
        FF_FILE* pFile = NULL;
        TIME(pFile = FF_Open(pIoman, filename, FF_MODE_READ, &error));
        FF_T_SINT32 read = 0;
        size_t total_len = 0;

        for (int block = 0; block < 1; ++block) {
            for (int kb = 0; kb < 256; kb++) {
                read = FF_Read(pFile, 1, sizeof(buffer), buffer);
                total_len += read;
            }
        }

        EXPECT(total_len, sizeof(buffer)*256*1);
        CHECK(FF_Close(pFile));
    }

    TEAR_DOWN;
}

static void TEST_CheckOpenReadClose()
{
    SETUP_TEST;

    for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        const char* filename = files[i];
        DEBUG(1, "\tfiles[i] : %s", filename);
        FF_FILE* pFile = NULL;
        TIME(pFile = FF_Open(pIoman, filename, FF_MODE_READ, &error));
        FF_T_SINT32 read = 0;
        size_t total_len = 0;

        for (int block = 0; block < 1; ++block) {
            const uint8_t* p = (uint8_t*)0x00100000;
            for (int kb = 0; kb < 256; kb++) {
                FF_T_SINT32 len = sizeof(buffer);
                read = FF_Read(pFile, 1, sizeof(buffer), buffer);
                total_len += read;
           
                const uint8_t* start = p;
                uint8_t fail = 0;
                do
                {
                    if (*p == buffer[read-len])
                    {
                        if (fail)
                        {
                            MSG_debug(0, __FILE__, __LINE__, "\t[%04x] STOPS FAILING!", p-start);
                            fail = 0;
                        }
                    }
                    else
                    {
                        if (!fail)
                        {
                            MSG_debug(0, __FILE__, __LINE__, "\t[%04x] STARTS FAILING!", p-start);
                            fail = 1;
                        }
                    }
                    p++;
                }
                while(--len);
            }
        }

        EXPECT(total_len, sizeof(buffer)*256*1);
        CHECK(FF_Close(pFile));
    }

    TEAR_DOWN;
}

static __attribute__((unused)) void TEST_RmFiles()
{
    SETUP_TEST;

    for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        const char* filename = files[i];
        DEBUG(1, "\tfiles[i] : %s", filename);
        CHECK(FF_RmFile(pIoman, filename));
    }

    TEAR_DOWN;
}


void RunFullFatTests()
{
    FF_ERROR error = FF_ERR_NONE;

    TIME(TEST_Setup());
    TIME(TEST_SetInfoNoCheck());

    // Test I/O speed
    {
        TIME(TEST_MkDir());
        TIME(TEST_OpenWriteClose());
        TIME(TEST_TimeOpenReadClose());

        TIME(TEST_GetInfo());

        TIME(TEST_RmDirTree());
    }

    TIME(TEST_VerifyInfo());

    // Test I/O correctness
    {
        TIME(TEST_MkDir());
        TIME(TEST_OpenWriteClose());
        TIME(TEST_CheckOpenReadClose());
    	TIME(TEST_RmFiles());
    	TIME(TEST_RmDir());
        TIME(TEST_RmDirTree());
    }

    TIME(TEST_VerifyInfo());

    TIME(TEST_Teardown());
}
