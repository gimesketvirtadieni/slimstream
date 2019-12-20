/*
 * Copyright 2017, Andrej Kislovskij
 *
 * This is PUBLIC DOMAIN software so use at your own risk as it comes
 * with no warranties. This code is yours to share, use and modify without
 * any restrictions or obligations.
 *
 * For more information see conwrap/LICENSE or refer refer to http://unlicense.org
 *
 * Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)
 */

#pragma once

#include <utility>  // std::as_const

#include "slim/util/buffer/Array.hpp"


namespace slim
{
namespace util
{
namespace buffer
{
template
<
    typename ElementType,
    template <typename> class StorageType = HeapBuffer
>
class RingViewPolicy : protected ArrayViewPolicy<ElementType, StorageType>
{
    public:
        using SizeType  = typename ArrayViewPolicy<ElementType, StorageType>::SizeType;
        using IndexType = typename ArrayViewPolicy<ElementType, StorageType>::IndexType;

        inline explicit RingViewPolicy(const typename StorageType<ElementType>::SizeType& s)
        : ArrayViewPolicy<ElementType, StorageType>{s} {}

        inline const auto& operator[](const IndexType& i) const
        {
            return ArrayViewPolicy<ElementType, StorageType>::operator[](normalizeIndex(head + i));
        }

        inline auto& operator[](const IndexType& i)
        {
            return const_cast<ElementType&>(std::as_const(*this)[i]);
        }

        inline void clear()
        {
            size = 0;
        }

        inline const auto getCapacity() const
        {
            return ArrayViewPolicy<ElementType, StorageType>::getSize();
        }

        inline const SizeType getSize() const
        {
            return size;
        }

        inline const auto isEmpty() const
        {
            return size == 0;
        }

        inline const auto isFull() const
        {
            return size == StorageType<ElementType>::getSize();
        }

        inline void pop()
        {
            if (isEmpty())
            {
                return;
            }

            size--;
            head = normalizeIndex(++head);
        }

        inline void push(const ElementType& item)
        {
            if (!isFull())
            {
                size++;
            }
            else
            {
                head = normalizeIndex(head + 1);
            }
            StorageType<ElementType>::getData()[normalizeIndex(head + size - 1)] = item;
        }

    protected:
        inline const auto getAbsoluteIndex(const IndexType& i) const
        {
            return i < StorageType<ElementType>::getSize() ? normalizeIndex(head + i) : i;
        }

        inline const auto normalizeIndex(const IndexType& i) const
        {
            return i < StorageType<ElementType>::getSize() ? i : i - StorageType<ElementType>::getSize();
        }

    private:
        IndexType head{0};
        SizeType  size{0};
};

template
<
    typename ElementType,
    template <typename> class StorageType = HeapBuffer,
    template <typename, template <typename> class> class BufferAccessPolicyType = RingViewPolicy
>
class Ring : public Array<ElementType, StorageType, BufferAccessPolicyType>
{
    public:
        inline explicit Ring(const typename StorageType<ElementType>::SizeType& c)
        : Array<ElementType, StorageType, BufferAccessPolicyType>{c} {}
};

}
}
}
