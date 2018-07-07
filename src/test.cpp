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

    int rst = 0;
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
    cout << "================Test TestAppendBlocksToFile=================" << endl;
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
