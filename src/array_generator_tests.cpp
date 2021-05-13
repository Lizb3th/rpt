// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "detail/array_generator.hpp"

#include <array>
#include <memory_resource>
#include <numeric>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using rpt::event_detail::array_generator;

namespace rpt::event_detail_tests {

    TEST_CLASS(array_generator_tests) {
	public:
		TEST_METHOD(construct);
		TEST_METHOD(construct_alloc);
		TEST_METHOD(make_array);
		TEST_METHOD(copy_push_back_array);

		TEST_METHOD(copy_remove_array);
		TEST_METHOD(big_array);
	};

	void array_generator_tests::construct()
	{
		array_generator<int*>();
	}

	void array_generator_tests::construct_alloc()
	{
		std::array<std::uint8_t, 304> buffer{};
		std::pmr::monotonic_buffer_resource mem_resource(buffer.data(),
														 buffer.size());
		auto generator = array_generator<int*>(&mem_resource);

		Assert::IsTrue(mem_resource.is_equal(*generator.get_allocator().resource()));
	}

	void array_generator_tests::make_array()
	{
		auto generator = array_generator<int*>{};
		
		int test = 2;

		auto array = generator.make_array(test);

		Assert::AreEqual(generator.get_size(array), 1ull);
		Assert::AreEqual(generator.get_generation(array), 1ull);
		Assert::AreEqual(generator.get_data(array)[0], &test);
	}

	void array_generator_tests::copy_push_back_array()
	{
		auto generator = array_generator<int*>{};

		int test1 = 2;
		int test2 = 4;

		auto array1 = generator.make_array(test1);
		auto array2 = generator.copy_push_back(array1, test2);

		Assert::AreEqual(1ull, generator.get_size(array1));
		Assert::AreEqual(1ull, generator.get_generation(array1));
		Assert::AreEqual(&test1, generator.get_data(array1)[0]);

		Assert::AreEqual(2ull, generator.get_size(array2));
		Assert::AreEqual(2ull, generator.get_generation(array2));
		Assert::AreEqual(&test1, generator.get_data(array2)[0]);
		Assert::AreEqual(&test2, generator.get_data(array2)[1]);
	}

	void array_generator_tests::copy_remove_array()
	{
		auto generator = array_generator<int*>{};

		int test1 = 2;
		int test2 = 4;

		auto array1 = generator.make_array(test1);
		auto array2 = generator.copy_push_back(array1, test2);
		auto array3 = generator.copy_remove(array2, test1);
		auto array4 = generator.copy_remove(array2, test2);

		Assert::AreEqual(2ull, generator.get_size(array2));
		Assert::AreEqual(2ull, generator.get_generation(array2));
		Assert::AreEqual(&test1, generator.get_data(array2)[0]);
		Assert::AreEqual(&test2, generator.get_data(array2)[1]);

		Assert::AreEqual(1ull, generator.get_size(array3));
		Assert::AreEqual(3ull, generator.get_generation(array3));
		Assert::AreEqual(&test2, generator.get_data(array3)[0]);

		Assert::AreEqual(1ull, generator.get_size(array4));
		Assert::AreEqual(3ull, generator.get_generation(array4));
		Assert::AreEqual(&test1, generator.get_data(array4)[0]);
	}

	void array_generator_tests::big_array()
	{
		auto generator = array_generator<int*>{};

		std::array<int, 10> tests = { 1,2,3,5,7,11,13,17,19,23 };

		auto big_array = std::accumulate(std::next(tests.begin()),
										 tests.end(),
										 generator.make_array(tests.front()),
										 [&](const auto& acc, auto& test) {
											return generator.copy_push_back(acc, test);
										 });

		Assert::AreEqual(10ull, generator.get_size(big_array));
	}

}