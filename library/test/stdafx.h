// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// TODO: reference additional headers your program requires here
#include <gtest/gtest.h>
#include <eventual/eventual.h>

template class eventual::future<int>;
template class eventual::future<int&>;

template class eventual::shared_future<int>;
template class eventual::shared_future<int&>;

template class eventual::promise<int>;
template class eventual::promise<int&>;

template class eventual::packaged_task<void(int)>;
template class eventual::packaged_task<int(int)>;
