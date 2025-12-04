#pragma once

namespace cache
{
	struct Line
	{
		int tag;
		int LRU;
		bool dirty;
	};

	class Way
	{
	private:
		Line* data;
		int size;

	public:
		Way() : data(nullptr), size(0) {}

		~Way() 
		{
			delete[] data;
		}

		void init(int n)
		{
			if (data != nullptr)
			{
				delete[] data;
			}
			size = n;
			data = new Line[n];
			for (int i = 0; i < n; ++i)
			{
				data[i].tag = 0;
				data[i].LRU = 0;
				data[i].dirty = false;
			}
		}

		Line* getData() const
		{
			return data;
		}

		int getSize() const
		{
			return size;
		}
	};
	
}