/*
 * Inkscape::Util::List - managed linked list
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_UTIL_LIST_H
#define SEEN_INKSCAPE_UTIL_LIST_H

#include <cstddef>
#include <iterator>
#include "gc-managed.h"
#include "traits/reference.h"

namespace Inkscape {

namespace Util {

template <typename T>
struct ListCell : public GC::Managed<> {
    ListCell() {}
    ListCell(typename Traits::Reference<T>::RValue v, ListCell *n)
    : value(v), next(n) {}

    T value;
    ListCell *next;
};

template <typename T> class List;
template <typename T> class MutableList;

template <typename T>
bool is_empty(List<T> const &list);

template <typename T>
typename List<T>::reference first(List<T> const &list);

template <typename T>
List<T> const &rest(List<T> const &list);

template <typename T>
MutableList<T> &rest(MutableList<T> const &list);

template <typename T>
MutableList<T> const &set_rest(MutableList<T> const &list,
                               MutableList<T> const &rest);

template <typename T>
class List<T const> {
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef T const value_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename Traits::Reference<value_type>::LValue reference;
    typedef typename Traits::Reference<value_type>::RValue const_reference;
    typedef typename Traits::Reference<value_type>::Pointer pointer;

    List() : _cell(NULL) {}
    explicit List(const_reference value, List const &next=List())
    : _cell(new ListCell<T>(value, next._cell)) {}

    operator bool() const { return this->_cell; }

    reference operator*() const { return this->_cell->value; }
    pointer operator->() const { return &this->_cell->value; }

    bool operator==(List const &other) const {
        return this->_cell == other._cell;
    }
    bool operator!=(List const &other) const {
        return this->_cell != other._cell;
    }

    List &operator++() {
        this->_cell = this->_cell->next;
        return *this;
    }
    List operator++(int) {
        List old(*this);
        this->_cell = this->_cell->next;
        return old;
    }

    friend reference first<>(List const &);
    friend List const &rest<>(List const &);
    friend bool is_empty<>(List const &);

protected:
    ListCell<T> *_cell;
};

template <typename T>
class List : public List<T const> {
public:
    typedef T value_type;
    typedef typename Traits::Reference<value_type>::LValue reference;
    typedef typename Traits::Reference<value_type>::RValue const_reference;
    typedef typename Traits::Reference<value_type>::Pointer pointer;

    List() : List<T const>() {}
    explicit List(const_reference value, List const &next=List())
    : List<T const>(value, next) {}

    reference operator*() const { return this->_cell->value; }
    pointer operator->() const { return &this->_cell->value; }

    List &operator++() {
        this->_cell = this->_cell->next;
        return *this;
    }
    List operator++(int) {
        List old(*this);
        this->_cell = this->_cell->next;
        return old;
    }

    friend reference first<>(List const &);
    friend List const &rest<>(List const &);
    friend bool is_empty<>(List const &);
};

template <typename T>
class List<T &> {
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef T &value_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename Traits::Reference<value_type>::LValue reference;
    typedef typename Traits::Reference<value_type>::RValue const_reference;
    typedef typename Traits::Reference<value_type>::Pointer pointer;

    List() : _cell(NULL) {}
    List(const_reference value, List const &next=List())
    : _cell(new ListCell<T &>(value, next._cell)) {}

    operator bool() const { return this->_cell; }

    reference operator*() const { return this->_cell->value; }
    pointer operator->() const { return &this->_cell->value; }

    bool operator==(List const &other) const {
        return this->_cell == other._cell;
    }
    bool operator!=(List const &other) const {
        return this->_cell != other._cell;
    }

    List &operator++() {
        this->_cell = this->_cell->next;
        return *this;
    }
    List operator++(int) {
        List old(*this);
        this->_cell = this->_cell->next;
        return old;
    }

    friend reference first<>(List const &);
    friend List const &rest<>(List const &);
    friend bool is_empty<>(List const &);

protected:
    ListCell<T &> *_cell;
};

template <typename T>
class MutableList : public List<T> {
public:
    MutableList() {}
    explicit MutableList(typename List<T>::const_reference value,
                         MutableList const &next=MutableList())
    : List<T>(value, next) {}

    MutableList &operator++() {
        this->_cell = this->_cell->next;
        return *this;
    }
    MutableList operator++(int) {
        MutableList old(*this);
        this->_cell = this->_cell->next;
        return old;
    }

    friend MutableList &rest<>(MutableList const &);
    friend MutableList const &set_rest<>(MutableList const &,
                                         MutableList const &);
};

/** @brief Creates a (non-empty) linked list.
 * 
 * Creates a new linked list with a copy of the given value (\a first)
 * in its first element; the remainder of the list will be the list
 * provided as \a rest.
 *
 * The remainder of the list -- the "tail" -- is incorporated by
 * reference rather than being copied.
 *
 * The returned value can also be treated as an STL forward iterator.
 *
 * These lists are designed to store simple values like pointers,
 * references, and scalar values.  While they can be used to directly
 * store more complex objects, destructors for those objects will not
 * be called unless those objects derive from Inkscape::GC::Finalized.
 *
 * In general it's better to use lists to store pointers or references
 * to objects requiring finalization and manage object lifetimes separately.
 *
 * @see Inkscape::GC::Finalized
 *
 * cons() is synonymous with List<T>(first, rest), except that the
 * compiler will usually be able to infer T from the type of \a rest.
 *
 * If you need to create an empty list (which can, for example, be used
 * as an 'end' value with STL algorithms), call the List<> constructor
 * with no arguments, like so:
 *
 *  List<int>()
 *
 * @see List<>
 * @see is_empty<>
 *
 * @param first the value for the first element of the list
 * @param rest the rest of the list; may be an empty list
 *
 * @returns a new list
 *
 */
template <typename T>
inline List<T> cons(typename Traits::Reference<T>::RValue first,
                    List<T> const &rest)
{
    return List<T>(first, rest);
}

/** @brief Creates a (non-empty) linked list whose tail can be exchanged
 *         for another.
 *
 * Creates a new linked list, but one whose tail can be exchanged for
 * another later by using set_rest() or assignment through rest()
 * as an lvalue.  It's otherwise identical to the "non-mutable" form.
 *
 * This form of cons() is synonymous with MutableList<T>(first, rest),
 * except that the compiler can usually infer T from the type of \a rest.
 *
 * As with List<>, you can create an empty list like so:
 *
 *  MutableList<int>()
 *
 * @see MutableList<>
 * @see is_empty<>
 *
 * @param first the value for the first element of the list
 * @param rest the rest of the list; may be an empty list
 *
 * @returns a new list
 */
template <typename T>
inline MutableList<T> cons(typename Traits::Reference<T>::RValue first,
                           MutableList<T> const &rest)
{
    return MutableList<T>(first, rest);
}

/** @brief Returns true if the given list is empty.
 *
 * Returns true if the given list is empty.  This is equivalent
 * to !list.
 *
 * @param list the list
 *
 * @returns true if the list is empty, false otherwise.
 */
template <typename T>
inline bool is_empty(List<T> const &list) { return !list._cell; }

/** @brief Returns the first value in a linked list.
 *
 * Returns a reference to the first value in the list.  This
 * corresponds to the value of the first argument passed to cons().
 *
 * If the list holds mutable values (or references to them), first()
 * can be used as an lvalue.
 *
 * For example:
 * 
 *  first(list) = value;
 *
 * The results of calling this on an empty list are undefined.
 *
 * @see cons<>
 * @see is_empty<>
 *
 * @param list the list; cannot be empty
 *
 * @returns a reference to the first value in the list
 */
template <typename T>
inline typename List<T>::reference first(List<T> const &list) {
    return list._cell->value;
}

/** @brief Returns the remainder of a linked list after the first element.
 *
 * Returns the remainder of the list after the first element (its "tail").
 *
 * This will be the same as the second argument passed to cons().
 *
 * The results of calling this on an empty list are undefined.
 *
 * @see cons<>
 * @see is_empty<>
 *
 * @param list the list; cannot be empty
 *
 * @returns the remainder of the list
 */
template <typename T>
inline List<T> const &rest(List<T> const &list) {
    return reinterpret_cast<List<T> const &>(list._cell->next);
}

/** @brief Returns a reference to the remainder of a linked list after
 *         the first element.
 *
 * Returns a reference to the remainder of the list after the first
 * element (its "tail").  For MutableList<>, rest() can be used as
 * an lvalue, to set a new tail.
 *
 * For example:
 *
 *  rest(list) = other;
 *
 * Results of calling this on an empty list are undefined.
 *
 * @see cons<>
 * @see is_empty<>
 *
 * @param list the list; cannot be empty
 *
 * @returns a reference to the remainder of the list
 */
template <typename T>
inline MutableList<T> &rest(MutableList<T> const &list) {
    return reinterpret_cast<MutableList<T> &>(list._cell->next);
}

/** @brief Sets a new tail for an existing linked list.
 * 
 * Sets the tail of the given MutableList<>, corresponding to the
 * second argument of cons().
 *
 * Results of calling this on an empty list are undefined.
 *
 * @see rest<>
 * @see cons<>
 * @see is_empty<>
 *
 * @param list the list; cannot be empty
 * @param rest the new tail; corresponds to the second argument of cons()
 *
 * @returns the new tail
 */
template <typename T>
inline MutableList<T> const &set_rest(MutableList<T> const &list,
                                      MutableList<T> const &rest)
{
    list._cell->next = rest._cell;
    return reinterpret_cast<MutableList<T> &>(list._cell->next);
}

}

}

#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
