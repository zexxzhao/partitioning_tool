#ifndef __CSRLIST_H__
#define __CSRLIST_H__

#include <type_traits>
#include <vector>
#include "CSRListObject.hpp"
#include "CSRListIterator.hpp"

#include "GraphConverter.hpp"


template <typename T = std::size_t, typename U = T, typename std::enable_if_t<std::is_integral_v<U> or std::is_enum_v<U>>* Dummy_value = nullptr > struct CSRList;

template <typename List> struct is_CSRList : std::false_type {};
template <typename T, typename U> struct is_CSRList<CSRList<T, U> > : std::true_type {};
template <typename T, typename U> struct is_CSRList<const CSRList<T, U> > : std::true_type {};
template <typename T, typename U> struct is_CSRList<volatile CSRList<T, U> > : std::true_type {};
template <typename T, typename U> struct is_CSRList<const volatile CSRList<T, U> > : std::true_type {};

template<typename List> inline constexpr bool is_CSRList_v = is_CSRList<List>::value;


template <typename T,
    typename U,
    typename std::enable_if_t<std::is_integral_v<U> or std::is_enum_v<U>>* Dummy_value>
    struct CSRList
{
    using data_type = T;
    using size_type = U;
	using dummy_type = typename std::enable_if<std::is_integral<U>::value>::type*;
	using const_Iterator = CSRListIterator<const CSRList<T, U, Dummy_value>>;

	CSRList() : _data(0), _offset(1, 0) {}

	CSRList(const std::vector<T>& data, const std::vector<U>& index_element_range) :
		_data(data), _offset(index_element_range) {}

	CSRList(std::vector<T>&& data, std::vector<U>&& index_element_range) noexcept :
		_data(std::move(data)), _offset(std::move(index_element_range)) {}

	CSRList(const_Iterator begin, const_Iterator end) : CSRList() {
		for(auto it = begin; it != end; ++it) {
			this->push_back(it->data());
		}
	}

	CSRList(const CSRList<T, U, Dummy_value>& csrlist) : 
		CSRList(csrlist._data, csrlist._offset) {}

	CSRList(CSRList<T, U, Dummy_value>&& csrlist) noexcept : 
		CSRList(std::move(csrlist._data), std::move(csrlist._offset)) {}

	const CSRList<T, U, Dummy_value>& operator=(const CSRList<T, U, Dummy_value>& csrlist) {
		if(this != &csrlist) {
			_data = csrlist._data;
			_offset = csrlist._offset;
		}
		return *this;
	}

	const CSRList<T, U, Dummy_value>& operator=(CSRList<T, U, Dummy_value>&& csrlist) noexcept {
		if(this != &csrlist) {
			_data = std::move(csrlist._data);
			_offset = std::move(csrlist._offset);
		}
		return *this;
	}

	std::pair<size_t, size_t> range(size_t index) const {
		return {_offset.at(index), _offset.at(index + 1)};
	}

	std::vector<data_type> data(size_t index) const {
		return std::vector<data_type> (
			_data.begin() + _offset.at(index),
			_data.begin() + _offset.at(index + 1) );
	}

	const std::vector<data_type>& data() const noexcept {
		return _data;
	}

	std::vector<data_type>& data() noexcept {
		return _data;
	}


	const std::vector<size_type>& offset() const noexcept {
		return _offset;
	}

	std::vector<size_type>& offset() noexcept {
		return _offset;
	}

	std::vector<T> operator[](size_t index) const {
		return data(index);
	}

	template<typename Other>
	std::common_type_t<T, Other>
    push_back(const std::vector<Other>& entity) {
        data().insert(data().end(), entity.begin(), entity.end());
        offset().push_back(data().size());
		return {};
    }

	template <typename Other>
	std::common_type_t<T, Other>
    push_back(std::vector<T>&& entity) {
		if(data().empty()) {
			data() = std::move(entity);
		}
		else if(not entity.empty()) {
			data().insert(data().end(), 
				std::make_move_iterator(entity.begin()), 
				std::make_move_iterator(entity.end()));
			entity.clear();
		}
		offset().push_back(data().size());
		return {};
    }

	auto operator+(const CSRList& list) const {
		CSRList results(*this);
		_concatenate(results, list);
		return std::move(results);
	}

	const auto& operator+=(const CSRList& list) {
		_concatenate(*this, list);
		return *this;
	}

	size_type num_entities() const {
		return _offset.size() - 1;
	}

	size_type size() const {
		return num_entities();
	}

	const_Iterator iterator(size_type index) const {
		return const_Iterator(*this, index);
	}

	const_Iterator begin() const {
        return iterator(0);
	}

    const_Iterator end() const {
        return iterator(num_entities());
    }

	template<typename Data = T>
	std::enable_if_t<std::is_same_v<Data, U> and std::is_integral_v<Data>, CSRList> reverse() const {
		//TODO: find an inplace reversing method
		//std::map<T, std::vector<T>> graph_tmp;
		auto vector_length = 1 + *std::max_element(this->data().begin(), this->data().end());
		std::vector<std::vector<T>> graph_tmp(vector_length);
		std::size_t expected_bandwidth = 32;
		for(auto& cache: graph_tmp) {
			cache.reserve(expected_bandwidth);
		}
		for(auto it = this->begin(); it != this->end(); ++it) {
			for(auto index : it->data()) {
				//graph_tmp[index].insert(it->index());
				graph_tmp[index].push_back(it->index());
			}
		}

		CSRList results;
		auto max_index = (graph_tmp.size() ? graph_tmp.size() - 1: 0); // an ordered map stores the largest index in the end
		for(Data i = 0; i <= max_index and max_index != 0; ++i) {
			const auto it = graph_tmp.begin() + i;
			if(it != graph_tmp.end() and it->size() > 0) {
				//auto v = std::vector<Data>(it->second.begin(), it->second.end());
				results.push_back(*it);
			}
			else {
				results.push_back(std::vector<Data>({}));
			}
		}
		return results;
	}

    void clear() {
        _data.clear();
        _offset.assign(1, static_cast<U>(0));
    }
	private:
	static void _concatenate(CSRList& first, const CSRList& second)	{
		if(second.size() > 0) {
			auto original_num_entities = first.size();
			auto augmented_num_entities = second.size();

			auto& data = first.data();
			data.insert(data.end(), second.data().begin(), second.data().end());
			auto& offset = first.offset();
			auto increment = offset.back();
			offset.insert(offset.end(), second.offset().begin() + 1, second.offset().end());
			std::for_each(offset.begin() + original_num_entities + 1, offset.end(), 
				[&](auto& item) { item += increment; } );

			assert(first.size() == original_num_entities + augmented_num_entities);
		}
	}
	std::vector<data_type> _data;
	std::vector<size_type> _offset;
};



#endif // __CSRLIST_H__
