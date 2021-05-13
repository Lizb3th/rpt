// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "listener.hpp"
#include "event.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_tests {

    TEST_CLASS(listener_tests) {
	public:
		TEST_METHOD(destruct_callback);
	};

	void listener_tests::destruct_callback() {
		auto test_int = rpt::event<int> {};

		auto shared_value = std::make_shared<int>(0);
		auto weak_value = std::weak_ptr{ shared_value };

		auto test_int_value{ 0 };

		{
			Assert::IsFalse(weak_value.expired());

			auto token_int = rpt::listener(test_int, [shared_value = std::move(shared_value), &test_int_value](int i) {
				test_int_value += i;
			});

			Assert::IsFalse(weak_value.expired());

			Assert::IsTrue(test_int_value == 0);
			test_int(1);
			Assert::IsTrue(test_int_value == 1);
			test_int(-1);
			Assert::IsTrue(test_int_value == 0);
		}

		Assert::IsTrue(weak_value.expired());

		Assert::IsTrue(test_int_value == 0);
		test_int(1);
		Assert::IsTrue(test_int_value == 0);
	}

}