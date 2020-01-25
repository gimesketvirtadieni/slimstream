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

#include <cstddef>  // std::size_t
#include <memory>   // std::unique_ptr

namespace slim
{
namespace util
{
namespace buffer
{

template
<
    typename ElementType,
    typename PointerType = std::unique_ptr<ElementType[]>
>
class DefaultHeapBufferStorage
{
    public:
        using SizeType = std::size_t;

        inline explicit DefaultHeapBufferStorage(const SizeType& s)
        : data{new ElementType[s]}
        , size{s} {}

        inline explicit DefaultHeapBufferStorage(PointerType d, const SizeType& s)
        : data{std::move(d)}
        , size{s} {}

        inline auto* getData() const
        {
            return data.get();
        }

        inline const auto getSize() const
        {
            return size;
        }

    private:
        PointerType data;
        SizeType    size;
};

template
<
    typename ElementType,
    template <typename> class HeapBufferStorageType = DefaultHeapBufferStorage
>
using DefaultHeapBufferViewPolicy = HeapBufferStorageType<ElementType>;

template
<
    typename ElementType,
    template <typename> class StorageType = DefaultHeapBufferStorage,
    template <typename, template <typename> class> class BufferViewPolicyType = DefaultHeapBufferViewPolicy
>
using HeapBuffer = BufferViewPolicyType<ElementType, StorageType>;

}
}
}
