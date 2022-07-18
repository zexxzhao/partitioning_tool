#ifndef __CSRLIST_ITERATOR_H__
#define __CSRLIST_ITERATOR_H__

#include <type_traits>


template <typename List> struct CSRListObject;

template<typename List, 
	typename data_type = typename List::data_type, 
	typename size_type = typename List::size_type,
	typename directed_tag = typename List::directed_category> 
struct CSRListIterator
{

	CSRListIterator(List& list, size_type index = static_cast<size_type>(0)) :
		_plist(&list), _begin(0), _end(list.num_entities()), _counter(index), _object(*_plist, _counter) {}
    
	CSRListIterator(const CSRListIterator<List>& iterator) :
		_plist(iterator._plist), _begin(iterator._begin), _end(iterator._end), _counter(iterator._counter), _object(*_plist, _counter) {}
	
	CSRListIterator(CSRListIterator<List>&&) noexcept = delete;

	const CSRListIterator<List>& operator=(const CSRListIterator<List>& iterator) {
		if(this != &iterator) {
			_begin = iterator.begin;
			_end = iterator.end;
			_counter = iterator._counter;
		}
		return *this;
	}

	const CSRListIterator<List>& operator=(CSRListIterator<List>&& iterator) noexcept = delete;

	auto operator+(size_type increment) const {
		return CSRListIterator<List>(*this->_plist, _counter + increment);
	}

	void operator+=(size_type increment) {
		_counter += increment;
		_object.index() += increment;
		assert(_begin <= _counter and _counter <= _end);
	}

	void operator++() {
		operator+=(static_cast<size_type>(1));
	}

	bool operator==(const CSRListIterator<List>& it) const {
		assert(_plist == it._plist);
		assert(_begin == it._begin);
		assert(_end == it._end);

		return _counter == it->_counter;
	}

	bool operator!=(const CSRListIterator<List>& it) const {
		assert(_plist == it._plist);
		assert(_begin == it._begin);
		assert(_end == it._end);

		return _counter != it._counter;
	}

    const CSRListObject<List>& operator*() const {
        return _object;
    }

	template<typename U = List>
	std::enable_if_t<not std::is_const_v<U>, CSRListObject<U>>& operator*() {
        return _object;
	}

    const CSRListObject<List>* operator->() const {
		return &_object;
	}

    template <typename U = List>
	std::enable_if_t<not std::is_const_v<U>, CSRListObject<U>>* operator->() {
        return &_object;
	}

	private:
	std::remove_reference_t<List>* _plist;
	size_type _begin;
	size_type _end;
	size_type _counter;

	CSRListObject<List> _object;
};


#endif // __CSRLIST_ITERATOR_H__