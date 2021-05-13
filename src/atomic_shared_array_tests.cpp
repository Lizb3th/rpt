// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "detail/atomic_shared_array.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_detail_tests {

	using rpt::event_detail::atomic_shared_array_t;
    
    TEST_CLASS(atomic_shared_array_tests) {
	public:
		TEST_METHOD(construct);

		TEST_METHOD(load_store);

		TEST_METHOD(compare_exchange);
	};

	void atomic_shared_array_tests::construct() {
		auto test = atomic_shared_array_t<int>{};
	}

	void atomic_shared_array_tests::load_store() {
		auto test_atomic = atomic_shared_array_t<int>{};

		Assert::IsTrue(test_atomic.load() == nullptr);

		auto test_ptr = std::shared_ptr{ std::make_unique<int[]>(10) };

		test_atomic.store(test_ptr);

		auto test_ptr_loaded = test_atomic.load();

		Assert::IsTrue(test_ptr_loaded == test_ptr);

		test_atomic.store(nullptr);

		Assert::IsTrue(test_atomic.load() == nullptr);
	}

	void atomic_shared_array_tests::compare_exchange() {
		auto test_atomic = atomic_shared_array_t<int>{};

		Assert::IsTrue(test_atomic.load() == nullptr);

		auto test_ptr = std::shared_ptr{ std::make_unique<int[]>(10) };

		test_atomic.store(test_ptr);

		Assert::AreEqual(test_ptr.use_count(), 2l);

		{
			auto compare = std::shared_ptr<int[]>{};

			Assert::IsFalse(test_atomic.compare_exchange_strong(compare, test_ptr));

			Assert::IsTrue(compare == test_ptr);

			Assert::AreEqual(test_ptr.use_count(), 3l);

			Assert::IsTrue(test_atomic.compare_exchange_strong(test_ptr, nullptr));

			Assert::AreEqual(test_ptr.use_count(), 2l);
		}

		Assert::IsTrue(test_atomic.load() == nullptr);

	}
}