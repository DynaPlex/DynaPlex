#pragma once
#include<vector>
#include<limits>
#include "dynaplex/error.h"


class FIFOQueue
{
	size_t first_item;
	size_t num_items;
	std::vector<int64_t> items;

	size_t GetVectorIndex(size_t& unlooped_index)
	{
		if (unlooped_index>=items.size())
		{
			return unlooped_index - items.size();
		}
		return unlooped_index;
	}
public:


	size_t Capacity() const
	{
		return items.size();
	}

	class const_iterator {
		size_t current;
		const FIFOQueue* queue_ptr;
	public:
		const_iterator(size_t current, const FIFOQueue* queue_ptr) : current(current), queue_ptr{ queue_ptr } {}

		bool operator!=(const_iterator& rhs) { return current != rhs.current; }
		const int64_t& operator*() {
			return queue_ptr->items[queue_ptr->GetVectorIndex(current)];
		}
		void operator++() {
			++current;
		}
	};

	const_iterator begin() const
	{
		return const_iterator(first_item, this);
	}

	const_iterator end() const
	{
		return const_iterator(first_item + num_items, this);
	}

	FIFOQueue() : FIFOQueue(0)
	{}

	FIFOQueue(size_t capacity, std::vector<int64_t> items = std::vector<int64_t>{}) : first_item{ 0 }, num_items{ 0 }, items(capacity)
	{
		if (capacity * 2 > std::numeric_limits<size_t>::max())
		{//Note that because of the wraparound logic, we need that size_t can fit twice the size of the maximum capacity..
			throw DynaPlex::Error("FIFOQueue: cannot fit this capacity");
		}
		for (int64_t t : items)
		{
			push_back(t);
		}
	}

	
	void push_back(int64_t item)
	{
		if (num_items == items.size())
		{
			//The queue is full. Thing is that the queue is implemented with a loop, so that adding an item and removing is very cheap (like it deque).
			//however, we use a 
		}
		size_t end = first_item + num_items;
		if (end >= items.size())
		{
			end -= items.size();
		}
		items[end] = item;
		num_items++;
	}

	int64_t& back()
	{
		if (IsEmpty())
		{
			throw DynaPlex::Error("FIFOQueue: queue is empty");
		}
		return items[GetVectorIndex(first_item+num_items-1)];
	}

	const int64_t& back() const
	{
		if (IsEmpty())
		{
			throw DynaPlex::Error("FIFOQueue: queue is empty");
		}
		return items[GetVectorIndex(first_item + num_items - 1)];
	}


	bool IsEmpty() const
	{
		return num_items == 0;
	}
	

	int64_t pop_front()
	{
		int64_t front = items[first_item];
		items[first_item++] = 0;
		num_items--;
		if (first_item == items.size())
		{
			first_item = 0;
		}
		return front;
	}
	int64_t& front()
	{
		if (IsEmpty())
		{
			throw DynaPlex::Error("FIFOQueue: queue is empty");
		}
		return items[first_item];
	}
	const int64_t& front() const
	{
		if (IsEmpty())
		{
			throw DynaPlex::Error("FIFOQueue: queue is empty");
		}
		return items[first_item];
	}



	friend bool operator==(const FIFOQueue& lhs, const FIFOQueue& rhs)
	{
		if (lhs.num_items != rhs.num_items)
		{
			return false;
		}
		auto lhsIter = lhs.begin();
		auto rhsIter = rhs.begin();
		auto lhsEnd = lhs.end();
		while (lhsIter != lhsEnd)
		{
			if (*(lhsIter) != *(rhsIter))
			{
				return false;
			}
			++lhsIter;
			++rhsIter;
		}
		return true;

	}

	friend bool operator!=(const FIFOQueue& lhs, const FIFOQueue& rhs)
	{
		return !(lhs == rhs);
	}
};

