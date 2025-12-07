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
		Way(int size);
		~Way();

		Line* getData() const;

		int getSize() const;

		int tag(int set) const;
	};
	
}