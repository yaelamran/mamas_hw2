#include "Way.h"

using namespace cache;

Way::Way(int size) : 
size(size)
{
    data = new Line[size];
    for (int i = 0; i < size; ++i)
    {
        data[i].tag = 0;
        data[i].LRU = 0;
        data[i].dirty = false;
    }
}

Way::~Way()
{
}

Line* Way::getData() const
{
    return nullptr;
}

int Way::getSize() const
{
    return 0;
}


int Way::tag(int set) const
{
    return 0;
}
