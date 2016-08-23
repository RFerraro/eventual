#pragma once

// The MIT License(MIT)
// 
// Copyright(c) 2016 Ryan Ferraro
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "future.h"
#include "shared_future.h"
#include "promise.h"
#include "packaged_task.h"
#include "eventual_methods.h"

#include "future.inl"
#include "shared_future.inl"
#include "promise.inl"
#include "packaged_task.inl"
#include "eventual_methods.inl"

#include "detail/BasicFuture.inl"
#include "detail/BasicPromise.inl"
#include "detail/BasicTask.inl"
#include "detail/CommonPromise.inl"
#include "detail/CompositeState.inl"
#include "detail/detail.inl"
#include "detail/FutureFactory.inl"
#include "detail/FutureHelper.inl"
#include "detail/memory_resource.inl"
#include "detail/polymorphic_allocator.inl"
#include "detail/resource_adapter.inl"
#include "detail/ResultBlock.inl"
#include "detail/SimpleDelegate.inl"
#include "detail/State.inl"
#include "detail/strong_polymorphic_allocator.inl"
#include "detail/utility.inl"
