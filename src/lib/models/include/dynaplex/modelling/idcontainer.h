#pragma once
#include <vector>
#include <optional>
#include "dynaplex/error.h"
#include "dynaplex/vargroup.h"
#include <string>
#include <algorithm>

namespace DynaPlex {

    template<Concepts::VarGroupConvertible T>
    class IdContainer {
    private:    
       std::vector<std::optional<std::pair<int64_t, T>>> backstore;
        size_t first_empty_id = 1;
        size_t num_items = 0;

        void updateFirstEmptyIndex() {
            while (first_empty_id < backstore.size() && backstore[first_empty_id].has_value()) {
                first_empty_id++;
            }
        }


    public:
        IdContainer() {}

        IdContainer(const VarGroup& varGroup) {
            auto keys = varGroup.Keys();
            for (const auto& key : keys) {
                if (!isNumeric(key)) {
                    throw Error("IdContainer: Key string is not numeric: " + key);
                }

                int64_t id = std::stoll(key);
                if (id >= backstore.size()) {
                    backstore.resize(id + 1);
                }
                if (id <= 0)
                    throw DynaPlex::Error("IdContainer:: item key/index should be >0.");

                DynaPlex::VarGroup vg;
                varGroup.Get(key, vg);
                backstore[id] = std::make_pair(id,T(vg));
                num_items++;
            }
            updateFirstEmptyIndex();
        }

        std::pair<int64_t, T>& AddNew() {
            if (first_empty_id >= backstore.size()) {
                backstore.resize(first_empty_id + 1);
            }
            auto& element = backstore[first_empty_id];
            // Placement new to construct the object in-place, to avoid a memory allocation here:
       
            backstore[first_empty_id].emplace(first_empty_id, T{});
            num_items++;
            auto id = first_empty_id;
            updateFirstEmptyIndex();
            return backstore[id].value();
        }

        bool HasId(int64_t id) const {
            return id >= 0 && id < static_cast<int64_t>(backstore.size()) && backstore[id].has_value();
        }

        const T& operator[](int64_t id) const {
            if (id < 0 || id >= static_cast<int64_t>(backstore.size()) || !backstore[id]) {
                throw Error("IdContainer::GetItem: Invalid ID or item does not exist");
            }
            return *(backstore[id].second);
        }

        T& operator[](int64_t id) {
            if (id < 0 || id >= static_cast<int64_t>(backstore.size()) || !backstore[id]) {
                throw Error("IdContainer::GetItem: Invalid ID or item does not exist");
            }
            return backstore[id].value().second;
        }

        void Delete(int64_t id) {
            if (id >0 && id < backstore.size() && backstore[id]) {
                backstore[id] = std::nullopt;
                num_items--;
                if (id < first_empty_id) {
                    first_empty_id = id;
                }
            }
            else {
                throw Error("IdContainer::DeleteItem : id not available");
            }
        }

        VarGroup ToVarGroup() const {
            VarGroup varGroup{};
            for (auto& [id,item] : *this) {
                varGroup.Add(std::to_string(id), item.ToVarGroup());                
            }
            return varGroup;
        }

        size_t size() const {
            return num_items;
        }

        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<int64_t, T>; 
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;  

            iterator(std::vector<std::optional<std::pair<int64_t, T>>>& bs, size_t pos) : backstore(bs), position(pos) {
                advanceToNextValid();
            }

            reference operator*() {
                return backstore[position].value();
            }

            iterator& operator++() {
                ++position;
                advanceToNextValid();
                return *this;
            }

            bool operator==(const iterator& other) const {
                return position == other.position;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

        private:
            std::vector<std::optional<std::pair<int64_t, T>>>& backstore;
            size_t position;

            void advanceToNextValid() {
                while (position < backstore.size() && !backstore[position]) {
                    ++position;
                }
            }
        };

        iterator begin() {
            return iterator(backstore, 0);
        }

        iterator end() {
            return iterator(backstore, backstore.size());
        }

        class const_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const std::pair<int64_t, T>;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator(const std::vector<std::optional<std::pair<int64_t, T>>>& bs, size_t pos)
                : backstore(bs), position(pos) {
                advanceToNextValid();
            }

            reference operator*() {
                return backstore[position].value();
            }

            const_iterator& operator++() {
                ++position;
                advanceToNextValid();
                return *this;
            }

            bool operator==(const const_iterator& other) const {
                return position == other.position;
            }

            bool operator!=(const const_iterator& other) const {
                return !(*this == other);
            }

        private:
            const std::vector<std::optional<std::pair<int64_t, T>>>& backstore;
            size_t position;

            void advanceToNextValid() {
                while (position < backstore.size() && !backstore[position]) {
                    ++position;
                }
            }
        };

        const_iterator begin() const {
            return const_iterator(backstore, 0);
        }

        const_iterator end() const {
            return const_iterator(backstore, backstore.size());
        }



    private:
        static bool isNumeric(const std::string& str) {
            if (str.empty() || (str.size() == 1 && (str[0] == '-' || str[0] == '+'))) {
                return false;
            }
            size_t startIndex = (str[0] == '-' || str[0] == '+') ? 1 : 0;
            return std::all_of(str.begin() + startIndex, str.end(), [](unsigned char c) { return std::isdigit(c); });
        }
    };

} // namespace DynaPlex