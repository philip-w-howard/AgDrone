
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

        if (block_start + block_size == start)
        {
            bool room_in_block = it->Append(size, data);
            if (!room_in_block) 
            {
                printf("Creating new data block\n");
                it++;
                if (it ==  m_data.end())
                    m_data.push_back(DataBlock(start, size, data));
                else
                    m_data.insert(it, DataBlock(start, size, data));
            }
            break;
        }

        it++;
    }

    if (it == m_data.end()) m_data.push_back(DataBlock(start, size, data));
}
//************************************************
bool DataList::GetHole(int *start, int *size)
{
    return false;
}
//************************************************
bool DataList::GetUnsentBlock(int *start, int *size, void **data)
{
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
