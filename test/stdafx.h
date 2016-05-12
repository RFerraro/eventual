// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// TODO: reference additional headers your program requires here
#include <gtest/gtest.h>
#include <eventual/eventual.h>

template class eventual::Future<void>;
template class eventual::Future<int>;
template class eventual::Future<int&>;

template class eventual::Shared_Future<void>;
template class eventual::Shared_Future<int>;
template class eventual::Shared_Future<int&>;

template class eventual::Promise<void>;
template class eventual::Promise<int>;
template class eventual::Promise<int&>;

template class eventual::Packaged_Task<void(int)>;
template class eventual::Packaged_Task<int(int)>;
