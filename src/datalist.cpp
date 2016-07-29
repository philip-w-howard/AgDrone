
#include <stdio.h>
#include <list>

#include "datablock.h"
#include "datalist.h"

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
void DataList::Insert(int start, int size, void *data)
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
            printf("Found duplicate block at %d %d : %d %d\n", 
                    start, size, block_start, block_size);
            return;
        }

        if (block_start + block_size == start)
        {
            bool room_in_block = it->Append(size, data);
            if (!room_in_block) 
            {
                printf("Creating new data block %d %d\n", start, size);
                it++;
                if (it ==  m_data.end())
                    m_data.push_back(DataBlock(start, size, data));
                else
                    m_data.insert(it, DataBlock(start, size, data));
            }
            return;
        }

        it++;
    }

    if (it == m_data.end()) 
    {
        printf("Creating new data block %d %d\n", start, size);
        m_data.push_back(DataBlock(start, size, data));
    }
}
//************************************************
bool DataList::GetHole(int *start, int *size)
{
    int prev_block_end = 0;
    std::list<DataBlock>::iterator it = m_data.begin();

    while (it != m_data.end())
    {
        if (it->Start() > prev_block_end)
        {
            *start = prev_block_end;
            *size = prev_block_end - it->Start();
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
        /*
        if (it->Start() > prev_block_end)
        {
            return false;
        }
        */

        // unsent?
        if (!it->IsSent())
        {
            it->MarkSent();
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
