// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "detail/event_base.hpp"

#include <vector>
#include <future>
#include <optional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_detail_tests {
    
	using rpt::event_detail::event_base;
	using rpt::event_detail::listener_base;

	TEST_CLASS(event_base_tests) {
	public:
		TEST_METHOD(construct);

		TEST_METHOD(empty);

		TEST_METHOD(add_remove_listener);

		TEST_METHOD(repudiate_delay);

		TEST_METHOD(clear_delay);
		
		TEST_METHOD(clear_one);

		TEST_METHOD(clear_many);

		TEST_METHOD(clear_complex);
	};

	void event_base_tests::construct() {
		auto test = event_base<>{};
	}

	void event_base_tests::empty() {
		auto test = event_base<>{};

		Assert::IsTrue(test.empty());
	}

	void event_base_tests::add_remove_listener() {
		auto test = event_base<>{};

		auto test_litener = listener_base<>{ { [](auto p) {} } };

		Assert::IsTrue(test.empty());

		test.subscribe(test_litener);

		Assert::IsFalse(test.empty());

		test.repudiate(test_litener);

		Assert::IsTrue(test.empty());
	}

	struct event_base_test_listener : listener_base<> {

		event_base_test_listener()
			: listener_base<>({
				[](auto& p) {},
				[](auto& p) { 
					static_cast<event_base_test_listener&>(p).attached = false;
					return true;
				}
			})
		{}

		bool attached{ true };
	};

	void event_base_tests::clear_one() {
		auto test = event_base<>{};

		auto test_litener = event_base_test_listener();

		test.subscribe(test_litener);

		Assert::IsFalse(test.empty());
		Assert::IsTrue(test_litener.attached);

		test.clear();

		//Assert::IsTrue(test.empty());
		Assert::IsFalse(test_litener.attached);
	}

	void event_base_tests::clear_many() {
		auto test = event_base<>{};

		auto test_liteners = std::vector<event_base_test_listener>(10, event_base_test_listener{} );

		for(auto& test_litener : test_liteners)
			test.subscribe(test_litener);

		Assert::IsFalse(test.empty());
		for (auto& test_litener : test_liteners)
			Assert::IsTrue(test_litener.attached);

		test.clear();

		//Assert::IsTrue(test.empty());		
		for (auto& test_litener : test_liteners)
			Assert::IsFalse(test_litener.attached);
	}

	void event_base_tests::repudiate_delay() {
		auto test = event_base<>{};

		auto test_litener = event_base_test_listener{};
		test.subscribe(test_litener);

		auto view = test.view_lock();

		Assert::AreEqual(1ull, view.size());

		std::thread([view = std::move(view)]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}).detach();

		auto time = std::chrono::system_clock::now();

		test.repudiate(test_litener);

		auto delay = std::chrono::system_clock::now() - time;

		Assert::IsTrue(delay > std::chrono::milliseconds(1));
	}

	void event_base_tests::clear_delay() {
		auto test = event_base<>{};

		auto test_litener = event_base_test_listener{};
		test.subscribe(test_litener);

		auto view = test.view_lock();

		Assert::AreEqual(1ull, view.size());

		std::thread([view = std::move(view)]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}).detach();

		auto time = std::chrono::system_clock::now();

		test.repudiate(test_litener);

		auto delay = std::chrono::system_clock::now() - time;

		Assert::IsTrue(delay > std::chrono::milliseconds(1));
	}

	void event_base_tests::clear_complex() {
		auto thread_count = std::thread::hardware_concurrency();

		auto test_liteners = std::vector<event_base_test_listener>(thread_count, event_base_test_listener{});
		auto times = std::vector<std::chrono::microseconds>();
		//auto threads = std::vector<std::thread>{};
		//threads.reserve(10);
		auto futures = std::vector<std::future<void>>();
		futures.reserve(thread_count);

		auto test = event_base<>{};
		auto view = std::make_optional(test.view_lock());



		try
		{

		std::mutex mutex;

		auto cv_mutex = std::mutex{};
		auto cv = std::condition_variable{};
		auto counter = std::atomic<int>{ 0 };

		for (auto& test_litener : test_liteners) {
			//threads.push_back(std::thread(
			futures.push_back(std::async(std::launch::async,
				[&]() mutable {
					counter++;
					{
						auto lock = std::lock_guard{ cv_mutex };
					}
					cv.notify_all();
					auto time = std::chrono::system_clock::now();
					test.subscribe(test_litener);
					auto lock = std::lock_guard(mutex);
					times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time));
					//auto view = test.view_lock();
			}));
		}

			{
				auto lock = std::lock_guard(mutex);
				Assert::IsTrue(times.empty());
			}

			//std::this_thread::sleep_for(std::chrono::milliseconds{ 40 });
			{
				auto waiter = std::unique_lock{ cv_mutex };
				cv.wait(waiter, [&] { return counter >= static_cast<std::size_t>(thread_count); });
			}

			while(test.view_lock().size() != thread_count) {
				std::this_thread::yield();
			}

			Assert::AreEqual<std::size_t>(test.view_lock().size(), thread_count);

			std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });

			view.reset();

			//for (auto& thread : threads)
			//	thread.join();

			for (auto& future : futures)
				future.wait();

			Assert::IsTrue(times.size() == thread_count);

			test_liteners.clear();

			//auto lock = std::lock_guard(mutex);

			//threads.clear();
			futures.clear();
		}
		catch (const std::exception& e)
		{
			auto what = e.what();
			assert(0);
		}

		for (auto& time : times)
			Assert::IsTrue(time > std::chrono::milliseconds{ 5 });

	}

}