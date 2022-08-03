#ifndef __CSRLIST_OBJECT_H__
#define __CSRLIST_OBJECT_H__

#include <vector>

template <typename List> struct CSRListObject {
    using data_type = typename List::data_type;
    using size_type = typename List::size_type;

    explicit CSRListObject(const List &list, size_type index)
        : _plist(&list), _index(index) {}

    std::pair<size_type, size_type> range() const {
        return {_plist->_offset.at(_index), _plist->_offset.at(_index + 1)};
    }

    std::vector<data_type> data() const {
        const auto it0 = _plist->data().begin() + _plist->offset().at(_index);
        const auto it1 =
            _plist->data().begin() + _plist->offset().at(_index + 1);
        return std::vector<data_type>(it0, it1);
    }

    size_type index() const { return _index; }

    size_type size() const {
        return _plist->offset().at(_index + 1) - _plist->offset().at(_index);
    }
    size_type &index() { return _index; }

    data_type operator[](size_type i) const {
        assert(i < size());
        auto shift = _plist->_offset.at(_index);
        return _plist->_data[i + shift];
    }

private:
    size_type _index;
    const List *_plist;
};

#endif // __CSRLIST_OBJECT_H__