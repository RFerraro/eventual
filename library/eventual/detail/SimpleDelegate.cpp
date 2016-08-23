#pragma once

#include "detail/SimpleDelegate.inl"
#include <cassert>

void eventual::detail::SimpleDelegate::operator()() const
{
    // this internal object should never be null
    assert(_object);

    _object->operator()();
}

void eventual::detail::SimpleDelegate::Callable::operator()()
{ }