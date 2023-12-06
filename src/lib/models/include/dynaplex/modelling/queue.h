#pragma once
#include<vector>
#include <limits>
#include <iterator>
#include "dynaplex/error.h"
#include "dynaplex/vargroup.h"




namespace DynaPlex {
	template<typename T>
	class Queue
	{
		static_assert(DynaPlex::Concepts::DP_ElementType<T>, " DynaPlex::Queue<T> - T must be of DP_ElementType, i.e. double, int64_t, string, or DynaPlex::VarGroupConvertible.");

	public:

		using value_type = T;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
	private:


		size_t first_item;
		size_t num_items;
		std::vector<T> items;

		size_t GetVectorIndex(const size_t& unlooped_index) const {
			if (unlooped_index >= items.size()) {
				return unlooped_index - items.size();
			}
			return unlooped_index;
		}

	public:
		size_t Capacity() const {
			return items.size();
		}



		class const_iterator {
		private:
			size_t current;
			const Queue* queue_ptr;
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;
			using const_pointer = const value_type*;

			const_iterator(size_t current, const Queue* queue_ptr)
				: current(current), queue_ptr(queue_ptr) {}

			bool operator!=(const const_iterator& rhs) const { return current != rhs.current; }
			bool operator==(const const_iterator& rhs) const { return current == rhs.current && queue_ptr == rhs.queue_ptr; }
			const T& operator*() {
				return queue_ptr->items[queue_ptr->GetVectorIndex(current)];
			}
			const_iterator& operator++() {
				++current;
				return *this;
			}
			const_iterator operator++(int) {
				const_iterator tmp = *this;
				++(*this);
				return tmp;
			}
		};

		const_iterator begin() const {
			return const_iterator(first_item, this);
		}

		const_iterator end() const {
			return const_iterator(first_item + num_items, this);
		}


		Queue(size_t n)
			: first_item{ 0 }, num_items{ n }, items(n, 0) {

		}


		Queue()
			: first_item{ 0 }, num_items{ 0 }, items(0) {}
		Queue(size_t n, const T value)
			: first_item{ 0 }, num_items{ n }, items(n, value) {}

		Queue(const Queue& other) = default;

		Queue(std::initializer_list<T> init)
			: first_item{ 0 }, num_items{ init.size() }, items(init) {}

		Queue(Queue&& other) noexcept
			: first_item{ other.first_item }, num_items{ other.num_items }, items(std::move(other.items)) {
			other.num_items = 0;
		}

		Queue& operator=(const Queue& other) = default;

		Queue& operator=(Queue&& other) noexcept {
			if (this != &other) {
				first_item = other.first_item;
				num_items = other.num_items;
				items = std::move(other.items);
				other.num_items = 0;
			}
			return *this;
		}

		void reserve(size_t capacity)
		{
			if (capacity > items.size())
			{
				std::vector<T> new_items(capacity);
				int i = 0;
				auto stop = end();
				for (auto it = begin(); it != stop; ++it) {
					new_items[i++] = *it;
				}
				first_item = 0;
				items = std::move(new_items);
			}
		}

		void push_back(T item) {

			if (num_items == items.size()) {
				//Expand the vector
				size_t new_capacity = (items.size() == 0) ? 4 : items.size() * 2;
				reserve(new_capacity);

			}
			items[GetVectorIndex(first_item + num_items++)] = item;
		}

		T& back() {
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			return items[GetVectorIndex(first_item + num_items - 1)];
		}

		const T& back() const {
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			return items[GetVectorIndex(first_item + num_items - 1)];
		}

		bool IsEmpty() const {
			return num_items == 0;
		}

		T pop_front() {
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			T front = items[first_item];
			items[first_item++] = T{};
			num_items--;
			if (first_item == items.size()) {
				first_item = 0;
			}
			return front;
		}

		T& front() {
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			return items[first_item];
		}

		const T& front() const {
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			return items[first_item];
		}

		T& at(size_t loc)
		{
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			if (loc >= num_items) {
				throw DynaPlex::Error("Queue: length error");
			}
			return items[GetVectorIndex(first_item + loc)];
		}

		const T& at(size_t loc) const
		{
			if (IsEmpty()) {
				throw DynaPlex::Error("Queue: queue is empty");
			}
			if (loc >= num_items) {
				throw DynaPlex::Error("Queue: length error");
			}
			return items[GetVectorIndex(first_item + loc)];
		}

		T sum()
		{
			static_assert(std::is_same_v<T, double> || std::is_same_v<T, int64_t>, "dynaplex::queue::sum can only be called when T is double or int64_t");

			T total = 0;
			auto stop = end();
			for (auto it = begin(); it != stop; ++it) {
				total += *it;
			}
			return total;
		}
		void clear()
		{
			items.clear();
			first_item = 0;
			num_items = 0;
		}

		friend bool operator==(const Queue<T>& lhs, const Queue<T>& rhs) {
			if (lhs.num_items != rhs.num_items) {
				return false;
			}
			return std::equal(lhs.begin(), lhs.end(), rhs.begin());
		}

		friend bool operator!=(const Queue<T>& lhs, const Queue<T>& rhs) {
			return !(lhs == rhs);
		}

	};

}