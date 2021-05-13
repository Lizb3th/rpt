// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "event.hpp"

#include <optional>
#include <array>
#include <memory_resource>
#include <memory>
#include <future>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_tests {

	TEST_CLASS(event_tests) {
	public:
		TEST_METHOD(construct);
		TEST_METHOD(invoke);

		TEST_METHOD(with_listener_simple);
		TEST_METHOD(subscribe_simple);
		TEST_METHOD(subscribe_simple2);

		TEST_METHOD(subscribe_multiple);

		TEST_METHOD(allocator_construct);

		TEST_METHOD(allocator_subscribe);

		TEST_METHOD(abuse_test);
	};

	void event_tests::construct() {
		auto test_void = rpt::event {};
		auto test_int = rpt::event<int> {};
		auto test_string = rpt::event<std::string> {};
	}

	void event_tests::invoke() {
		auto test_void = rpt::event {};
		test_void();

		auto test_int = rpt::event<int> {};
		test_int(8);

		auto test_string = rpt::event<std::string> {};
		test_string("test_str");

		auto test_int_string = rpt::event<int, std::string> {};
		test_int_string(0, "test_str");
	}

	template<typename Fn>
	std::optional<rpt::listener<Fn>> get_optional_litener(rpt::event<>& e, Fn&& fn)
	{
		return std::optional<rpt::listener<Fn>>{std::in_place, e, std::forward<Fn>(fn)};
	}

	template<typename Fn, typename... T>
	std::optional<rpt::listener<Fn, T...>> get_optional_litener(rpt::event<T...>& e, Fn&& fn)
	{
		return std::optional<rpt::listener<Fn, T...>>{std::in_place, e, std::forward<Fn>(fn)};
	}
	void event_tests::with_listener_simple() {
		auto test_void = rpt::event {};

		auto test_void_counter{ 0 };

		auto token_void = get_optional_litener(test_void, [&test_void_counter]() {
			test_void_counter++;
		});

		Assert::IsTrue(test_void_counter == 0);
		test_void();
		Assert::IsTrue(test_void_counter == 1);
		token_void.reset();
		Assert::IsTrue(test_void_counter == 1);
		test_void();
		Assert::IsTrue(test_void_counter == 1);
	}

	void event_tests::subscribe_simple() {
		auto test_void = rpt::event {};

		auto test_void_counter{ 0 };

		{
			auto token_void = test_void.subscribe([&test_void_counter]() {
				test_void_counter++;
				});

			Assert::IsTrue(test_void_counter == 0);
			test_void();
			Assert::IsTrue(test_void_counter == 1);
			test_void();
			Assert::IsTrue(test_void_counter == 2);
		}

		Assert::IsTrue(test_void_counter == 2);
		test_void();
		Assert::IsTrue(test_void_counter == 2);
	}

	void event_tests::subscribe_simple2() {
		auto test_int = rpt::event<int> {};

		auto test_int_value{ 0 };

		{
			auto token_int = test_int.subscribe([&test_int_value](auto i) {
				test_int_value += i;
				});

			Assert::IsTrue(test_int_value == 0);
			test_int(1);
			Assert::IsTrue(test_int_value == 1);
			test_int(-1);
			Assert::IsTrue(test_int_value == 0);
		}

		Assert::IsTrue(test_int_value == 0);
		test_int(1);
		Assert::IsTrue(test_int_value == 0);
	}

	void event_tests::subscribe_multiple() {
		auto test_int_value = std::atomic{ 0ll };

		auto test_int = rpt::event<int> {};

		{
			auto token_int0 = get_optional_litener(test_int, [&test_int_value](auto i) {
				test_int_value += i;
			});

			Assert::IsTrue(test_int_value == 0);
			test_int(1);
			Assert::IsTrue(test_int_value == 1);
			test_int(-1);
			Assert::IsTrue(test_int_value == 0);

			{
				auto token_int1 = test_int.subscribe([&test_int_value](auto i) {
					test_int_value += 2*i;
				});

				Assert::IsTrue(test_int_value == 0);
				test_int(1);
				Assert::IsTrue(test_int_value == 3);
				test_int(-1);
				Assert::IsTrue(test_int_value == 0);

				token_int0.reset();
				Assert::IsTrue(test_int_value == 0);
				test_int(1);
				Assert::IsTrue(test_int_value == 2);
				test_int(-1);
				Assert::IsTrue(test_int_value == 0);
			}

			Assert::IsTrue(test_int_value == 0);
			test_int(1);
			Assert::IsTrue(test_int_value == 0);
		}
	}

	void event_tests::allocator_construct() {

		std::array<std::uint8_t, 304> buffer{};
		std::pmr::monotonic_buffer_resource mem_resource(buffer.data(),
			buffer.size());

		auto e = event(&mem_resource);
	}

	void event_tests::allocator_subscribe() {

		std::array<std::uint8_t, 304> buffer{0};
		std::pmr::monotonic_buffer_resource mem_resource(buffer.data(),
			buffer.size());

		auto e = event(&mem_resource);

		auto pre_l_buffer_size = buffer.size() - std::count(buffer.begin(), buffer.end(), 0);

		decltype(pre_l_buffer_size) post_l_buffer_size;

		{
			auto l = e.subscribe([]() {
			});

			post_l_buffer_size = buffer.size() - std::count(buffer.begin(), buffer.end(), 0);

			Assert::AreNotEqual(post_l_buffer_size, pre_l_buffer_size);
		}

		auto post_destruct_l_buffer_size = buffer.size() - std::count(buffer.begin(), buffer.end(), 0);

		Assert::AreNotEqual(post_l_buffer_size, post_destruct_l_buffer_size);
	}

	void event_tests::abuse_test() {

		auto counter = std::atomic_llong{ 0 };

		auto e = event<int>{};

		auto listeners_prom = std::promise<void>{};
		auto listeners_ready = listeners_prom.get_future();

		auto event_prom = std::promise<void>{};
		auto event_fired = event_prom.get_future();

		auto thread = std::thread([
			&e,
			&counter,
			listeners_prom = std::move(listeners_prom),
			event_fired = std::move(event_fired)
			]() mutable {

			auto listener_mold = [&counter](auto i) {
				counter += i;
			};

			using listener_type = listener<decltype(listener_mold), int>;

			auto listener_array = std::array<std::unique_ptr<listener_type>, 500>{};

			std::generate(listener_array.begin(), listener_array.end(), [&e, &listener_mold]() {
				return std::make_unique<listener_type>(e, listener_mold);
			});

			listeners_prom.set_value();

			event_fired.wait();

			auto foo = 0;
		});

		thread.detach();

		listeners_ready.get();

		Assert::AreEqual(0ll, counter.load());

		e(1);

		Assert::AreEqual(500ll, counter.load());

		event_prom.set_value();
	}
}