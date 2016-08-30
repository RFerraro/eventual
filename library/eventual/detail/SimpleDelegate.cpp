#if defined(EVENTUAL_HEADER_ONLY)
#pragma once
#endif

#include "SimpleDelegate.inl"
#include <cassert>

eventual::detail::SimpleDelegate::Delegate::~Delegate() { }


void eventual::detail::SimpleDelegate::operator()() const
{
    // this internal object should never be null
    assert(_object);

    _object->operator()();
}

void eventual::detail::SimpleDelegate::Callable::operator()()
{ }