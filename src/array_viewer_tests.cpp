// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#include "pch.h"

#include "detail/array_viewer.hpp"
#include "detail/array_generator.hpp"

#include <array>
#include <numeric>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace rpt::event_detail_tests {

	using rpt::event_detail::array_viewer;

	TEST_CLASS(array_viewer_tests) {
	public:
		TEST_METHOD(construct);
		TEST_METHOD(move_construct_TODO) {}
	};

	template<typename T, typename U, auto s>
	std::shared_ptr<T*[]> make_array(std::array<U,s>& arr) {
		auto generator = event_detail::array_generator<T*>{};

		return std::accumulate(std::next(arr.begin()),
			arr.end(),
			generator.make_array(arr.front()),
			[&](const auto& acc, auto& test) {
				return generator.copy_push_back(acc, test);
			});
	}

	void array_viewer_tests::construct() {
		auto arr = std::array{ 1,2,3,5,7,11,13,17,19,23 };

		auto array = make_array<int>(arr);
		auto ptr = std::shared_ptr<decltype(array)::element_type[]>{ array, array.get() + 2 };
		auto view = array_viewer<int>(ptr, arr.size());

		Assert::IsTrue(std::equal(arr.begin(), arr.end(), view.begin(), view.end()));

		Assert::AreEqual(11, *std::next(view.begin(), 5));
		arr[5] = 29;
		Assert::AreEqual(29, *std::next(view.begin(), 5));
	}

}