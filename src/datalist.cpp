
#include <stdio.h>
#include <list>

#include "datablock.h"
#include "datalist.h"

#include "log.h"

//************************************************
DataList::DataList()
{
    DataBlock block;
    m_data.push_front(block);
    m_DataComplete = false;
}
//************************************************
DataList::~DataList()
{}

//************************************************
bool DataList::Insert(int start, int size, void *data)
{
    std::list<DataBlock>::iterator it = m_data.begin();
    int block_start;
    int block_size;

    while (it != m_data.end())
    {
        block_start = it->Start();
        block_size = it->Size();

        if (block_start < start && block_start + block_size > start)
        {
            // must be a duplicate block
            //WriteLog("Found duplicate block at %d %d : %d %d\n", 
            //        start, size, block_start, block_size);
            return false;
        }
        else if (block_start + block_size == start)
        {
            // belongs in this block

            // check to see if this is a duplicate of the beginning of the 
            // next block
            it++;
            if (start == it->Start()) return false; // duplicate
            it--;

            bool room_in_block = it->Append(size, data);
            if (!room_in_block) 
            {
                //WriteLog("Creating new data block %d %d\n", start, size);
                it++;
                if (it ==  m_data.end())
                    m_data.push_back(DataBlock(start, size, data));
                else
                    m_data.insert(it, DataBlock(start, size, data));
            }
            return true;
        }
        else if (block_start > start)
        {
            // belongs as new block before this one

            //WriteLog("Creating new data block %d %d\n", start, size);
            m_data.insert(it, DataBlock(start, size, data));
            return true;
        }

        it++;
    }

    if (it == m_data.end()) 
    {
        //WriteLog("Creating new data block %d %d\n", start, size);
        m_data.push_back(DataBlock(start, size, data));
    }

    return true;
}
//************************************************
bool DataList::GetHole(int *start, int *size, int start_addr)
{
    int lastBlock = -1;
    int prev_block_end = 0;
    std::list<DataBlock>::iterator it = m_data.begin();

    while (it != m_data.end())
    {
        if (lastBlock >= it->Start())
        {
            ListAllBlocks();
            exit(1);
        }
        if (it->Start() > prev_block_end && prev_block_end > start_addr)
        {
            *start = prev_block_end;
            *size = it->Start() - prev_block_end;
            return true;
        }

        prev_block_end = it->Start() + it->Size();

        it++;
    }

    *start = prev_block_end;
    *size = -1;

    return false;
}
//************************************************
bool DataList::GetUnsentBlock(int *start, int *size, void **data)
{
    int prev_block_end = 0;
    std::list<DataBlock>::iterator it = m_data.begin();

    while (it != m_data.end())
    {
        *start = it->Start();
        *size = it->Size();
        *data = it->Data();

        // look for a hole
        if (it->Start() > prev_block_end)
        {
            return false;
        }

        // unsent?
        if (!it->IsSent())
        {
            it->MarkSent();
            if (it != m_data.begin())
            {
                it--;
                //WriteLog("Erasing sent block %d %d\n", 
                //        it->Start(), it->Size());
                m_data.erase(it);
            }

            return true;
        }

        prev_block_end = it->Start() + it->Size();

        it++;
    }

    return false;
}
//************************************************
bool DataList::DataIsComplete()
{
    return m_DataComplete;
}

//************************************************
void DataList::SetDataComplete()
{
    m_DataComplete = true;
}
//************************************************
void DataList::ListAllBlocks()
{
    int lastBlock = -1;
    std::list<DataBlock>::iterator it = m_data.begin();

    while (it != m_data.end())
    {
        if (lastBlock >= it->Start())
        {
            WriteLog("Data List out of order %d %d\n", lastBlock, it->Start());
            fprintf(stderr, "Data List out of order %d %d\n", 
                    lastBlock, it->Start());
        }

        if (it->IsSent())
            WriteLog("Block %d %d sent\n", it->Start(), it->Size());
        else
            WriteLog("Block %d %d\n", it->Start(), it->Size());
        it++;
    }
    fflush(stdout);
}
