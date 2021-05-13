// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "detail/listener_base.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_detail_tests {

	using rpt::event_detail::listener_base;

    TEST_CLASS(listener_base_tests) {
	public:
		TEST_METHOD(construct);
		TEST_METHOD(invoke_empty);
		TEST_METHOD(invoke_int);
		TEST_METHOD(invoke_string);

		TEST_METHOD(detatch);
	};
    
	void listener_base_tests::construct() {
		auto test_empty = listener_base<>{ { [](auto p) {} } };
		auto test_int = listener_base<int>{ { [](auto p, auto i) {} } };
		auto test_double = listener_base<double>{ { [](auto p, auto d) {} } };
		auto test_pack = listener_base<int, int, double, std::string>{ {
			[](auto, int, int, double, std::string) {}
		} };
	}

	template<typename... T>
	struct test_listener : listener_base<T...>
	{
		template<typename CB>
		test_listener(CB&&);

		template<typename CB, typename Detatch>
		test_listener(CB&&, Detatch&&);

		int				i_value{};
		std::string		s_value{};

		bool			dt_value{};
	};

	template<typename... T>
	template<typename CB>
	test_listener<T...>::test_listener(CB&& f)
		: listener_base<T...>{ {std::move(f)} }
	{
	}


	template<typename... T>
	template<typename CB, typename Detatch>
	test_listener<T...>::test_listener(CB&& f, Detatch&& dt)
		: listener_base<T...>{ {std::move(f)}, std::move(dt) }
	{
	}

	void listener_base_tests::invoke_empty() {

		auto test_empty = test_listener<>({ [](auto& p) {
			static_cast<test_listener<>&>(p).i_value++;
		} });

		Assert::AreEqual(0, test_empty.i_value);
		test_empty();
		Assert::AreEqual(1, test_empty.i_value);
	}

	void listener_base_tests::invoke_int() {

		auto test_int = test_listener<int>({ [](auto& p, auto i) {
			static_cast<test_listener<int>&>(p).i_value = i;
		} });

		Assert::AreEqual(0, test_int.i_value);
		test_int(1);
		Assert::AreEqual(1, test_int.i_value);
		test_int(3);
		Assert::AreEqual(3, test_int.i_value);
		test_int(0);
		Assert::AreEqual(0, test_int.i_value);
	}

	void listener_base_tests::invoke_string() {

		auto test_string = test_listener<std::string>({ [](auto& p, auto s) {
			static_cast<test_listener<std::string>&>(p).s_value = s;
		} });

		Assert::IsTrue(test_string.s_value.empty());
		test_string(std::string{ "1" });
		Assert::AreEqual(std::string("1"), test_string.s_value);
		test_string(std::string{ "ONE" });
		Assert::AreEqual(std::string("ONE"), test_string.s_value);
		test_string(std::string_view{ "SV" });
		Assert::AreEqual(std::string("SV"), test_string.s_value);
	}

	void listener_base_tests::detatch() {

		bool called = false;

		auto test_empty = test_listener<int>({ 
			[](auto& p, auto i) {
				static_cast<test_listener<int>&>(p).i_value += i;
			},
			[](auto& p) {
				if (static_cast<test_listener<int>&>(p).dt_value == true)
					return true;
				
				static_cast<test_listener<int>&>(p).dt_value = true;
				return false;
			}
		});


		Assert::AreEqual(0, test_empty.i_value);
		Assert::AreEqual(false, test_empty.dt_value);

		test_empty(4);

		Assert::AreEqual(4, test_empty.i_value);
		Assert::AreEqual(false, test_empty.dt_value);

		auto detatch = test_empty.detatch();

		Assert::AreEqual(4, test_empty.i_value);
		Assert::AreEqual(true, test_empty.dt_value);
		Assert::AreEqual(false, detatch);

		detatch = test_empty.detatch();

		Assert::AreEqual(4, test_empty.i_value);
		Assert::AreEqual(true, test_empty.dt_value);
		Assert::AreEqual(true, detatch);
	}
}