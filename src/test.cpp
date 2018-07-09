#include <iostream>
#include <cstring>
#include "VHDController.h"
#include "BSFSParams.h"
#include "test.h"
#include "FSController.h"
#include "iNode.h"

using namespace std;

void TestVHDController(VHDController& vhdc)
{
    cout << "================Test VHDController=================" << endl;
    // Test VHD Create
    cout << "================Test VHD Create=================" << endl;
    if (!vhdc.Exists())
    {
        cout << "Initializing VHD file " << vhdc.Getfilename() << "...";
        if (vhdc.Create())
            cout << "Done" << endl;
        else
            cout << "Error" << endl;
    }
    cout << "Loading VHD file " << vhdc.Getfilename() << "...";
    if (vhdc.Load())
        cout << "Done" << endl;
    else
        cout << "Error" << endl;
    // Test VHD R/W
    /*cout << "================Test VHD R/W=================" << endl;
    char wbuff[BLOCKSIZE];
    memset(wbuff, 0, BLOCKSIZE);
    strcpy(wbuff, "Hello world");
    vhdc.WriteBlock(0, wbuff);
    vhdc.WriteBlock(BLOCKNUM - 1, wbuff, 5);
    bool a = true;
    vhdc.WriteBlock(1, (char*)&a, sizeof(bool));
    char rbuff[BLOCKSIZE];
    vhdc.ReadBlock(0, rbuff);
    cout << rbuff << endl;
    vhdc.ReadBlock(BLOCKNUM - 1, rbuff);
    cout << rbuff << endl;*/
}

void TestFSController(FSController& fsc)
{
    cout << "================Test FSController=================" << endl;
    // Test Format
    cout << "================Test Format=================" << endl;
    if (fsc.IsFormat())
        cout << "Already formatted" << endl;
    else
    {
        if (!fsc.Format())
            cout << "Format failed" << endl;
        else
            cout << "Format successful" << endl;
    }
}

void TestGLP(FSController& fsc)
{
    // Test GLP Distribute
    cout << "================Test GLP Distribute=================" << endl;
    bid_t bid;
    int blockCnt = 0;
    while (fsc.fbc.Distribute(&bid))
    {
        //cout << "Block " << bid << " distributed" << endl;
        blockCnt++;
    }
    cout << "Distributed block cnt: " << blockCnt << endl;
    cout << "Distributed block cnt expected: " << DATABLOCK_MAX - DATABLOCK_MIN + 1 << endl;

    // Test GLP Recycle
    blockCnt = 0;
    for (int i = DATABLOCK_MIN; i <= DATABLOCK_MAX; i++)
    {
        if (fsc.fbc.Recycle(i))
        {
            //cout << "Block " << i << " recycled" << endl;
            blockCnt++;
        }
    }
    cout << "Recycled block cnt: " << blockCnt << endl;
    cout << "Recycled block cnt expected: " << DATABLOCK_MAX - DATABLOCK_MIN + 1 << endl;
}

void TestGetBIDByFOff(FSController& fsc)
{
    cout << "================Test GetBIDByFOff=================" << endl;
    bool flag = true;
    // Create iNode
    iNode testiNode;
    testiNode.data[0] = 233;    // 2
    testiNode.data[5] = 666;    // 5120
    testiNode.data[9] = 22;     // 9300
    testiNode.data[10] = 33;    // 10250  271370
    testiNode.data[11] = 44;    // 272390 67380230
    // Create level one indirect index block
    char buf[BLOCKSIZE];    //33
    int target1 = 321;
    int target2 = 123;
    memcpy(buf, &target1, sizeof(int));
    memcpy(buf + 255 * sizeof(int), &target2, sizeof(int));
    flag &= fsc.vhdc.WriteBlock(33, buf);
    // Create level two indirect index block
    char buf1[BLOCKSIZE];   //44
    char buf2[BLOCKSIZE];   //55
    int target3 = 55, target4 = 66;
    memcpy(buf1, &target3, sizeof(int));
    memcpy(buf2, &target4, sizeof(int));
    char buf5[BLOCKSIZE];   //77
    char buf6[BLOCKSIZE];   //88
    int target5 = 77, target6 = 88;
    memcpy(buf1 + 255 * sizeof(int), &target5, sizeof(int));
    memcpy(buf5 + 255 * sizeof(int), &target6, sizeof(int));
    flag &= fsc.vhdc.WriteBlock(44, buf1);
    flag &= fsc.vhdc.WriteBlock(55, buf2);
    flag &= fsc.vhdc.WriteBlock(77, buf5);
    flag &= fsc.vhdc.WriteBlock(88, buf6);

    bid_t rst = 0;
    fsc.GetBIDByFOff(testiNode, 2, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 5120, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 9300, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 10250, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 271370, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 272390, &rst);
    cout << rst << endl;
    fsc.GetBIDByFOff(testiNode, 67380230, &rst);
    cout << rst << endl;

    cout << flag << endl;
}

void TestReadFileToBuf(FSController& fsc)
{
    cout << "================Test ReadFileToBuf=================" << endl;
    // Create iNode
    iNode testiNode;
    testiNode.size = 1024*3;
    testiNode.data[1] = 111;
    testiNode.data[2] = 222;
    char buf[BLOCKSIZE];
    for (int i = 0; i < BLOCKSIZE; ++i)
        buf[i] = '!';
    fsc.vhdc.WriteBlock(111, buf);
    fsc.vhdc.WriteBlock(222, buf);

    // start  len
    // 1030   10
    char testBuf1[10];
    fsc.ReadFileToBuf(testiNode, 1030, 10, testBuf1);
    for (int i = 0; i < 10; ++i)
        cout << testBuf1[i];
    cout << endl;

    // start  len
    // 1024   1024
    int cnt = 0;
    char testBuf2[1024];
    fsc.ReadFileToBuf(testiNode, 1024, 1024, testBuf2);
    for (int i = 0; i < 1024; ++i)
        if (testBuf2[i] == '!') ++cnt;
    cout << cnt << endl;

    // start  len
    // 1030   1024
    cnt = 0;
    char testBuf3[1024];
    fsc.ReadFileToBuf(testiNode, 1030, 1024, testBuf3);
    for (int i = 0; i < 1024; ++i)
    {
        if (testBuf3[i] == '!') ++cnt;
    }
    cout << cnt << endl;

    // start  len
    // 1030   2042
    cnt = 0;
    char testBuf4[2042];
    fsc.ReadFileToBuf(testiNode, 1030, 2042, testBuf4);
    for (int i = 0; i < 2042; ++i)
    {
        if (testBuf3[i] == '!') ++cnt;
    }
    cout << cnt << endl;
}

void TestAppendBlocksToFile(FSController &fsc)
{
    cout << "================Test AppendBlocksToFile=================" << endl;
    iNode testiNode;
    testiNode.size = 0;
    testiNode.blocks = 0;
    testiNode.bytes = 0;
    fsc.ifbc.Distribute(&testiNode.bid);
    cout << "testiNode.bid: " << testiNode.bid << endl;
    fsc.vhdc.WriteBlock(testiNode.bid, (char*)&testiNode, sizeof(iNode));

    if (fsc.AppendBlocksToFile(testiNode, 30000))
        cout << "Successfully appended" << endl;
    else
        cout << "Failed to append" << endl;

    cout << "testiNode.blocks: " << testiNode.blocks << endl;
    for (int i = 0; i < 12; i++)
        cout << "testiNode.data[" << i << "]: " << testiNode.data[i] << endl;
}

void TestWriteFileFromBuf(FSController &fsc)
{
    cout << "================Test WriteFileFromBuf=================" << endl;
    iNode testiNode;
    testiNode.size = 0;
    testiNode.blocks = 0;
    testiNode.bytes = 0;
    fsc.ifbc.Distribute(&testiNode.bid);
    cout << "testiNode.bid: " << testiNode.bid << endl;
    int i = 0;
    char s[1024 * 270 + 10];
    for (i = 0; i < 1024 * 270 + 1; i++)
        s[i] = 'b';
    s[i] = '\0';
    if (fsc.WriteFileFromBuf(testiNode, 0, strlen(s), s))
        cout << "Write successful" << endl;
    else
        cout << "Write failed" << endl;
    cout << "testiNode.blocks: " << testiNode.blocks << endl;
    cout << "testiNode.bytes: " << testiNode.bytes << endl;
    cout << "testiNode.size: " << testiNode.size << endl;
    cout << "testiNode.atime: " << testiNode.atime << endl;
    cout << "testiNode.mtime: " << testiNode.mtime << endl;
    for (i = 0; i < 12; i++)
        cout << "testiNode.data[" << i << "]: " << testiNode.data[i] << endl;
}

void TestCreateRootDir(FSController& fsc)
{
    cout << "================Test CreateRootDir=================" << endl;
    if (!fsc.CreateRootDir())
        cout << "Failed to Create /" << endl;
    else
        cout << "/ Created" << endl;
    iNode rootiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    cout << "rootiNode.size: " << rootiNode.size << endl;
    for (int i = 0; i < 12; i++)
        cout << "rootiNode.data[" << i << "]: " << rootiNode.data[i] << endl;
}

void TestCreateSubDir(FSController& fsc)
{
    cout << "================Test CreateSubDir=================" << endl;
    iNode rootiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    char dirname[] = "home";
    if (!fsc.CreateSubDir(rootiNode, dirname, OWNER_ALLFLAG | PUBLIC_ALLFLAG, ROOT_UID))
        cout << "Failed to Create /home" << endl;
    else
        cout << "/home Created" << endl;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    cout << "rootiNode.size: " << rootiNode.size << endl;
    for (int i = 0; i < 12; i++)
        cout << "rootiNode.data[" << i << "]: " << rootiNode.data[i] << endl;
}

void TestParsePath(FSController& fsc)
{
    cout << "================Test ParsePath=================" << endl;
    char path[MAX_CMD_LEN] = "/home/../../../../../../home";
    iNode rootiNode;
    iNode rstiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &rstiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }
    else
    {
        cout << "Parse " << path << " complete" << endl;
        cout << rstiNode.name << endl;
    }
}

void TestGetAbsDir(FSController& fsc)
{
    cout << "================Test GetAbsDir=================" << endl;
    char path[MAX_CMD_LEN] = "/home";
    iNode rootiNode;
    iNode rstiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &rstiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    char rstPath[MAX_CMD_LEN];
    fsc.GetAbsDir(rstiNode, rstPath);
    cout << "Abs Dir: " << rstPath << endl;
}

void TestTouch(FSController& fsc)
{
    cout << "================Test Touch=================" << endl;
    char path[MAX_CMD_LEN] = "/home";
    iNode rootiNode;
    iNode homeiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &homeiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    // Touch /home/1.txt
    char fname[] = "1.txt";
    iNode rstiNode;
    if (!fsc.Touch(homeiNode, fname, FILE_DEFAULT_FLAG, ROOTDIRiNODE, &rstiNode))
    {
        cout << "Failed to touch " << fname << endl;
    }
    else
    {
        cout << "Touch " << fname << " success" << endl;
    }
}

void TestGetContentInDir(FSController& fsc)
{
    cout << "================Test GetContentInDir=================" << endl;
    char path[MAX_CMD_LEN] = "/home";
    iNode rootiNode;
    iNode homeiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &homeiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    SFD* SFDList = new SFD[homeiNode.size / sizeof(SFD)];
    if (!fsc.GetContentInDir(homeiNode, SFDList))
    {
        cout << "Failed to ls /home" << endl;
    }
    else
    {
        for (int i = 0; i < (int)(homeiNode.size / sizeof(SFD)); i++)
        {
            cout << SFDList[i].name << endl;
        }
    }
    delete[] SFDList;
}

void TestWriteFileFromBuf2(FSController& fsc)
{
    cout << "================Test WriteFileFromBuf2=================" << endl;
    char path[MAX_CMD_LEN] = "/home/1.txt";
    iNode rootiNode;
    iNode txtiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &txtiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    char s[1024 * 270];
    for (int i = 0; i < 1024 * 270; i++)
        s[i] = 'y';
    if (!fsc.WriteFileFromBuf(txtiNode, 0, 1024 * 270, s))
    {
        cout << "Failed to write " << path << endl;
    }
    else
    {
        cout << "Write " << path << " success" << endl;
        cout << "txtiNode.size: " << txtiNode.size << endl;
        for (int i = 0; i < 12; i++)
            cout << "txtiNode.data[" << i << "]: " << txtiNode.data[i] << endl;
    }
}

void TestDeleteFile(FSController& fsc)
{
    cout << "================Test DeleteFile=================" << endl;
    char path[MAX_CMD_LEN] = "/home/1.txt";
    iNode rootiNode;
    iNode txtiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &txtiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    if (!fsc.DeleteFile(txtiNode))
        cout << "Failed to delete " << path << endl;
    else
    {
        cout << "Delete " << path << " success" << endl;
    }
}

void TestDeleteDir(FSController& fsc)
{
    cout << "================Test DeleteDir=================" << endl;
    char path[MAX_CMD_LEN] = "/home";
    iNode rootiNode;
    iNode homeiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &homeiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    if (!fsc.DeleteDir(homeiNode))
        cout << "Failed to delete " << path << endl;
    else
    {
        cout << "Delete " << path << " success" << endl;
        fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
        SFD* SFDList = new SFD[rootiNode.size / sizeof(SFD)];
        if (!fsc.GetContentInDir(rootiNode, SFDList))
        {
            cout << "Failed to ls /" << endl;
        }
        else
        {
            for (int i = 0; i < (int)(rootiNode.size / sizeof(SFD)); i++)
            {
                cout << SFDList[i].name << endl;
            }
        }
        delete[] SFDList;
    }
}

void TestCopyFile(FSController& fsc)
{
    cout << "================Test CopyFile=================" << endl;
    char path[MAX_CMD_LEN] = "/home/1.txt";
    iNode rootiNode;
    iNode txtiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &txtiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    char targetPath[MAX_CMD_LEN] = "/home";
    iNode homeiNode;
    if (!fsc.ParsePath(rootiNode, targetPath, true, &homeiNode))
    {
        cout << "Failed to parse " << targetPath << endl;
        return;
    }

    char newName[FILENAME_MAXLEN] = "2.txt";
    if (!fsc.Copy(txtiNode, homeiNode, newName, ROOT_UID))
    {
        cout << "Failed to copy" << endl;
    }
    else
    {
        cout << "Copy from " << path << " to " << newName << " success" << endl;
    }
}

void TestCopyDir(FSController& fsc)
{
    cout << "================Test CopyFile=================" << endl;
    char path[MAX_CMD_LEN] = "/home/";
    iNode rootiNode;
    iNode homeiNode;
    fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode);
    if (!fsc.ParsePath(rootiNode, path, true, &homeiNode))
    {
        cout << "Failed to parse " << path << endl;
        return;
    }

    //
}
