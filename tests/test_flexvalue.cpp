#include <chrono>
#include <gtest/gtest.h>

#include <flexvalue/FlexValue.hpp>

using namespace FlexValue;

namespace {
#define UNREFERENCED(P)          (P)
}
// ============================================================
// ЧАСТЬ 1: БАЗОВЫЕ ТИПЫ И КОНСТРУКТОРЫ (адаптированы)
// ============================================================
TEST(FlexValueTest, NullValue)
{
	Value v = make_null();
	EXPECT_TRUE(v.is_null());
	EXPECT_TRUE(v.is<std::monostate>());
}

TEST(FlexValueTest, BoolValue)
{
	Value v{ true };
	EXPECT_TRUE(v.is<bool>());
	EXPECT_EQ(v.as<bool>(), true);

	Value v2{ false };
	EXPECT_EQ(v2.as<bool>(), false);
	EXPECT_NE(v, v2);
}

TEST(FlexValueTest, IntegerTypes)
{
	Value i32{ int32_t{-42} };
	EXPECT_TRUE(i32.is<int32_t>());
	EXPECT_EQ(i32.as<int32_t>(), -42);

	Value u32{ uint32_t{100} };
	EXPECT_TRUE(u32.is<uint32_t>());
	EXPECT_EQ(u32.as<uint32_t>(), 100);

	Value i64{ int64_t{-9223372036854775807LL} };
	EXPECT_TRUE(i64.is<int64_t>());
	EXPECT_EQ(i64.as<int64_t>(), -9223372036854775807LL);

	Value u64{ uint64_t{18446744073709551615ULL} };
	EXPECT_TRUE(u64.is<uint64_t>());
	EXPECT_EQ(u64.as<uint64_t>(), 18446744073709551615ULL);
}

TEST(FlexValueTest, FloatingPointTypes)
{
	Value f{ 3.14f };
	EXPECT_TRUE(f.is<float>());
	EXPECT_FLOAT_EQ(f.as<float>(), 3.14f);

	Value d{ 2.718281828 };
	EXPECT_TRUE(d.is<double>());
	EXPECT_DOUBLE_EQ(d.as<double>(), 2.718281828);
}

TEST(FlexValueTest, StringType)
{
	Value s{ std::string{"hello"} };
	EXPECT_TRUE(s.is<std::string>());
	EXPECT_EQ(s.as<std::string>(), "hello");

	Value s2{ "world" };
	EXPECT_TRUE(s2.is<std::string>());
	EXPECT_EQ(s2.as<std::string>(), "world");
}

TEST(FlexValueTest, FractionType)
{
	Value frac = make_fraction(30, 1);
	EXPECT_TRUE(frac.is<Fraction>());
	EXPECT_EQ(frac.as<Fraction>().num, 30);
	EXPECT_EQ(frac.as<Fraction>().den, 1);

	Fraction f2{ 25, 2 };
	Value v2{ f2 };
	EXPECT_EQ(v2.as<Fraction>(), f2);
}

TEST(FlexValueTest, EnumAndFlags)
{
	Value e = make_enum(5, "MyEnum");
	EXPECT_TRUE(e.is<EnumValue>());
	EXPECT_EQ(e.as<EnumValue>().value, 5);
	EXPECT_EQ(e.as<EnumValue>().type_name, "MyEnum");

	Value f = make_flags(0x0F, "MyFlags");
	EXPECT_TRUE(f.is<FlagsValue>());
	EXPECT_EQ(f.as<FlagsValue>().value, 0x0F);
	EXPECT_EQ(f.as<FlagsValue>().type_name, "MyFlags");
}

// ============================================================
// ЧАСТЬ 2: КОНТЕЙНЕРЫ: ARRAY (адаптированы)
// ============================================================
TEST(FlexValueTest, EmptyArray)
{
	Value arr = make_array();
	EXPECT_TRUE(arr.is<CowBox<Array>>());
	EXPECT_EQ(array_size(arr), 0);
}

TEST(FlexValueTest, ArrayWithItems)
{
	std::vector<Value> items;
	items.push_back(Value{ int32_t{1} });
	items.push_back(Value{ int32_t{2} });
	items.push_back(Value{ int32_t{3} });

	Value arr = make_array(std::move(items));
	EXPECT_EQ(array_size(arr), 3);
	EXPECT_EQ(arr.as<CowBox<Array>>().get().items[0].as<int32_t>(), 1);
	EXPECT_EQ(arr.as<CowBox<Array>>().get().items[2].as<int32_t>(), 3);
}

TEST(FlexValueTest, ArrayPush)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 10 });
	array_push(arr, std::string{ "test" });
	array_push(arr, true);

	EXPECT_EQ(array_size(arr), 3);
	EXPECT_EQ(arr.as<CowBox<Array>>().get().items[0].as<int32_t>(), 10);
	EXPECT_EQ(arr.as<CowBox<Array>>().get().items[1].as<std::string>(), "test");
	EXPECT_EQ(arr.as<CowBox<Array>>().get().items[2].as<bool>(), true);
}

// ============================================================
// ЧАСТЬ 3: КОНТЕЙНЕРЫ: MAP (адаптированы)
// ============================================================
TEST(FlexValueTest, EmptyMap)
{
	Value m = make_map();
	EXPECT_TRUE(m.is<CowBox<Map>>());
	EXPECT_EQ(m.as<CowBox<Map>>().get().fields.size(), 0);
}

TEST(FlexValueTest, MapWithFields)
{
	Value m = make_map({
		{"width",  Value{int32_t{1920}}         },
		{"height", Value{int32_t{1080}}         },
		{"name",   Value{std::string{"Full HD"}}}
		});

	EXPECT_EQ(m.as<CowBox<Map>>().get().fields.size(), 3);
	EXPECT_EQ(map_get(m, "width").as<int32_t>(), 1920);
	EXPECT_EQ(map_get(m, "height").as<int32_t>(), 1080);
	EXPECT_EQ(map_get(m, "name").as<std::string>(), "Full HD");
}

TEST(FlexValueTest, MapSetAndGet)
{
	Value m = make_map();
	map_set(m, "bitrate", uint32_t{ 5000000 });
	map_set(m, "format", std::string{ "I420" });

	EXPECT_EQ(map_get(m, "bitrate").as<uint32_t>(), 5000000);
	EXPECT_EQ(map_get(m, "format").as<std::string>(), "I420");
}

TEST(FlexValueTest, MapKeyNotFound)
{
	Value m = make_map();
	EXPECT_THROW(map_get(m, "nonexistent"), KeyNotFoundException);
}

TEST(FlexValueTest, MapGetExistingKey)
{
	Value m = make_map();
	map_set(m, "key", int32_t{ 42 });

	EXPECT_EQ(map_get(m, "key").as<int32_t>(), 42);
}

TEST(FlexValueTest, MapSetCreatesNewKey)
{
	Value m = make_map();
	map_set(m, "new_key", std::string{ "value" });

	EXPECT_EQ(map_get(m, "new_key").as<std::string>(), "value");
}

TEST(FlexValueTest, MapSetStrictFailsOnMissingKey)
{
	Value m = make_map();
	map_set(m, "existing", int32_t{ 10 });

	EXPECT_TRUE(map_set_strict(m, "existing", 20));
	EXPECT_EQ(map_get(m, "existing").as<int32_t>(), 20);

	EXPECT_FALSE(map_set_strict(m, "missing", 99));
	EXPECT_THROW(map_get(m, "missing"), KeyNotFoundException);
}

// ============================================================
// ЧАСТЬ 4: РЕКУРСИВНАЯ ВЛОЖЕННОСТЬ (адаптированы)
// ============================================================
TEST(FlexValueTest, NestedArrayInMap)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });

	Value m = make_map();
	map_set(m, "numbers", arr);

	const auto& nums = map_get(m, "numbers");
	EXPECT_EQ(array_size(nums), 2);
}

TEST(FlexValueTest, NestedMapInArray)
{
	Value inner = make_map({
		{"x", Value{int32_t{10}}},
		{"y", Value{int32_t{20}}}
		});
	Value arr = make_array();
	array_push(arr, inner);

	EXPECT_EQ(array_size(arr), 1);
	const auto& item = arr.as<CowBox<Array>>().get().items[0];
	EXPECT_EQ(map_get(item, "x").as<int32_t>(), 10);
}

TEST(FlexValueTest, DeepNesting)
{
	Value deep = make_map();
	Value arr1 = make_array();
	Value inner_map = make_map();
	Value arr2 = make_array();
	array_push(arr2, int32_t{ 42 });
	map_set(inner_map, "data", arr2);
	array_push(arr1, inner_map);
	map_set(deep, "level1", arr1);

	const auto& l1 = map_get(deep, "level1");
	EXPECT_EQ(array_size(l1), 1);
	const auto& im = l1.as<CowBox<Array>>().get().items[0];
	const auto& a2 = map_get(im, "data");
	EXPECT_EQ(array_size(a2), 1);
	EXPECT_EQ(a2.as<CowBox<Array>>().get().items[0].as<int32_t>(), 42);
}

// ============================================================
// ЧАСТЬ 5: СРАВНЕНИЯ (адаптированы)
// ============================================================
TEST(FlexValueTest, EqualityScalars)
{
	Value v1{ int32_t{100} };
	Value v2{ int32_t{100} };
	Value v3{ int32_t{200} };

	EXPECT_EQ(v1, v2);
	EXPECT_NE(v1, v3);
}

TEST(FlexValueTest, EqualityStrings)
{
	Value s1{ std::string{"test"} };
	Value s2{ std::string{"test"} };
	Value s3{ std::string{"other"} };

	EXPECT_EQ(s1, s2);
	EXPECT_NE(s1, s3);
}

TEST(FlexValueTest, EqualityArrays)
{
	Value arr1 = make_array();
	array_push(arr1, int32_t{ 1 });
	array_push(arr1, int32_t{ 2 });

	Value arr2 = make_array();
	array_push(arr2, int32_t{ 1 });
	array_push(arr2, int32_t{ 2 });

	Value arr3 = make_array();
	array_push(arr3, int32_t{ 1 });
	array_push(arr3, int32_t{ 3 });

	EXPECT_EQ(arr1, arr2);
	EXPECT_NE(arr1, arr3);
}

TEST(FlexValueTest, EqualityMaps)
{
	Value m1 = make_map({
		{"a", Value{int32_t{1}}},
		{"b", Value{int32_t{2}}}
		});
	Value m2 = make_map({
		{"a", Value{int32_t{1}}},
		{"b", Value{int32_t{2}}}
		});
	Value m3 = make_map({
		{"a", Value{int32_t{1}}},
		{"b", Value{int32_t{3}}}
		});

	EXPECT_EQ(m1, m2);
	EXPECT_NE(m1, m3);
}

TEST(FlexValueTest, EqualityDifferentTypes)
{
	Value i{ int32_t{10} };
	Value s{ std::string{"10"} };
	EXPECT_NE(i, s);
}

// ============================================================
// ЧАСТЬ 6: КОПИРОВАНИЕ И ПЕРЕМЕЩЕНИЕ (адаптированы)
// ============================================================
TEST(FlexValueTest, CopyValue)
{
	Value orig = make_map({
		{"key", Value{std::string{"value"}}}
		});
	Value copy = orig;

	EXPECT_EQ(orig, copy);
	map_set(copy, "key2", int32_t{ 123 });
	EXPECT_NE(orig, copy);
}

TEST(FlexValueTest, MoveValue)
{
	Value orig = make_array();
	array_push(orig, int32_t{ 42 });
	Value moved = std::move(orig);

	EXPECT_EQ(array_size(moved), 1);
	EXPECT_EQ(moved.as<CowBox<Array>>().get().items[0].as<int32_t>(), 42);
}

TEST(FlexValueTest, CowBoxCopySemantics)
{
	Array a1;
	a1.items.push_back(Value{ int32_t{10} });
	CowBox<Array> box1{ std::move(a1) };

	CowBox<Array> box2 = box1;
	EXPECT_EQ(box1, box2);

	box2.get().items.push_back(Value{ int32_t{20} });
	EXPECT_NE(box1, box2);
}

// ============================================================
// ЧАСТЬ 7: ОБРАБОТКА ОШИБОК (обновлены)
// ============================================================
TEST(FlexValueTest, BadVariantAccess)
{
	Value v{ int32_t{100} };
	EXPECT_THROW(v.as<std::string>(), TypeMismatchException);
}

TEST(FlexValueTest, WrongContainerType)
{
	Value v{ int32_t{100} };
	EXPECT_THROW(array_push(v, int32_t{ 1 }), NotContainerException);
	EXPECT_THROW(map_set(v, "key", int32_t{ 1 }), NotContainerException);
}

TEST(FlexValueTest, ArraySizeOnNonArray)
{
	Value v = make_map();
	EXPECT_THROW(array_size(v), NotContainerException);
}

// ============================================================
// ЧАСТЬ 8: TO_STRING ДЛЯ ОТЛАДКИ (адаптированы)
// ============================================================
TEST(FlexValueTest, ToStringScalars)
{
	EXPECT_EQ(to_string(make_null()), "null");
	EXPECT_EQ(to_string(Value{ true }), "true");
	EXPECT_EQ(to_string(Value{ int32_t{42} }), "42");
	EXPECT_EQ(to_string(Value{ std::string{"test"} }), "\"test\"");
}

TEST(FlexValueTest, ToStringFraction)
{
	Value frac = make_fraction(25, 2);
	EXPECT_EQ(to_string(frac), "25/2");
}

TEST(FlexValueTest, ToStringArray)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });
	std::string s = to_string(arr);
	EXPECT_NE(s.find("1"), std::string::npos);
	EXPECT_NE(s.find("2"), std::string::npos);
}

TEST(FlexValueTest, ToStringMap)
{
	Value m = make_map({
		{"x", Value{int32_t{10}}}
		});
	std::string s = to_string(m);
	EXPECT_NE(s.find("x"), std::string::npos);
	EXPECT_NE(s.find("10"), std::string::npos);
}

// ============================================================
// ЧАСТЬ 9: DVB СЦЕНАРИИ (адаптированы на uint32_t)
// ============================================================
TEST(FlexValueTest, DVB_PAT_Scenario)
{
	Value pat = make_map({
		{"table_id",            Value{uint32_t{0x00}}  },
		{"pid",                 Value{uint32_t{0x0000}}},
		{"version_number",      Value{uint32_t{1}}     },
		{"transport_stream_id", Value{uint32_t{1234}}  }
		});

	Value programs = make_array();
	array_push(
		programs,
		make_map({
			{"program_number", Value{uint32_t{1}}  },
			{"pid",            Value{uint32_t{256}}}
			})
	);
	array_push(
		programs,
		make_map({
			{"program_number", Value{uint32_t{2}}  },
			{"pid",            Value{uint32_t{512}}}
			})
	);
	map_set(pat, "programs", programs);

	EXPECT_EQ(map_get(pat, "table_id").as<uint32_t>(), 0x00);
	EXPECT_EQ(map_get(pat, "transport_stream_id").as<uint32_t>(), 1234);

	const auto& progs = map_get(pat, "programs");
	EXPECT_EQ(array_size(progs), 2);

	const auto& prog1 = progs.as<CowBox<Array>>().get().items[0];
	EXPECT_EQ(map_get(prog1, "program_number").as<uint32_t>(), 1);
	EXPECT_EQ(map_get(prog1, "pid").as<uint32_t>(), 256);
}

TEST(FlexValueTest, DVB_PMT_WithDescriptors)
{
	Value pmt = make_map({
		{"table_id",       Value{uint32_t{0x02}}},
		{"program_number", Value{uint32_t{1}}   },
		{"pcr_pid",        Value{uint32_t{100}} }
		});

	Value prog_descs = make_array();
	array_push(
		prog_descs,
		make_map({
			{"tag",          Value{uint32_t{0x09}}  },
			{"ca_system_id", Value{uint32_t{0x0B00}}}
			})
	);
	map_set(pmt, "program_descriptors", prog_descs);

	Value components = make_array();
	Value comp1 = make_map({
		{"stream_type",    Value{uint32_t{0x1B}}},
		{"elementary_pid", Value{uint32_t{101}} },
		{"descriptors",    make_array()         }
		});
	array_push(components, comp1);
	map_set(pmt, "components", components);

	EXPECT_EQ(map_get(pmt, "table_id").as<uint32_t>(), 0x02);
	EXPECT_EQ(map_get(pmt, "pcr_pid").as<uint32_t>(), 100);

	const auto& comps = map_get(pmt, "components");
	EXPECT_EQ(array_size(comps), 1);
	const auto& c = comps.as<CowBox<Array>>().get().items[0];
	EXPECT_EQ(map_get(c, "stream_type").as<uint32_t>(), 0x1B);
}

TEST(FlexValueTest, DVB_ComplexNesting)
{
	Value service = make_map({
		{"service_id",   Value{uint32_t{100}}              },
		{"service_name", Value{std::string{"Test Channel"}}}
		});

	Value events = make_array();
	Value event1 = make_map({
		{"event_id",   Value{uint32_t{1}}         },
		{"start_time", Value{uint64_t{1234567890}}},
		{"duration",   Value{uint32_t{3600}}      }
		});

	Value event_descs = make_array();
	Value short_event = make_map({
		{"tag",        Value{uint32_t{0x4D}}                       },
		{"language",   Value{std::string{"eng"}}                   },
		{"event_name", Value{std::string{"News"}}                  },
		{"text",       Value{std::string{"Evening news broadcast"}}}
		});
	array_push(event_descs, short_event);
	map_set(event1, "descriptors", event_descs);

	array_push(events, event1);
	map_set(service, "events", events);

	const auto& evts = map_get(service, "events");
	EXPECT_EQ(array_size(evts), 1);
	const auto& e = evts.as<CowBox<Array>>().get().items[0];
	EXPECT_EQ(map_get(e, "event_id").as<uint32_t>(), 1);

	const auto& descs = map_get(e, "descriptors");
	EXPECT_EQ(array_size(descs), 1);
	const auto& d = descs.as<CowBox<Array>>().get().items[0];
	EXPECT_EQ(map_get(d, "tag").as<uint32_t>(), 0x4D);
	EXPECT_EQ(map_get(d, "event_name").as<std::string>(), "News");
}

// ============================================================
// ЧАСТЬ 10: COW (COPY-ON-WRITE) СЕМАНТИКА
// ============================================================
TEST(FlexValueCOW, CowBoxShallowCopy)
{
	Array arr;
	arr.items.push_back(Value{ int32_t{42} });
	CowBox<Array> box1{ std::move(arr) };

	CowBox<Array> box2 = box1;
	EXPECT_EQ(box1.ref_count(), 2);
	EXPECT_EQ(box2.ref_count(), 2);
}

TEST(FlexValueCOW, CowBoxCopyOnWrite)
{
	Array arr;
	arr.items.push_back(Value{ int32_t{10} });
	CowBox<Array> box1{ std::move(arr) };

	EXPECT_EQ(box1.ref_count(), 1);

	CowBox<Array> box2 = box1;
	EXPECT_EQ(box1.ref_count(), 2);
	EXPECT_EQ(box2.ref_count(), 2);

	box2.get().items.push_back(Value{ int32_t{20} });

	EXPECT_EQ(box1.ref_count(), 1);
	EXPECT_EQ(box2.ref_count(), 1);
	EXPECT_EQ(box1.get().items.size(), 1);
	EXPECT_EQ(box2.get().items.size(), 2);
}

TEST(FlexValueCOW, ValueCopySharingMemory)
{
	Value v1 = make_map({
		{"key", Value{std::string{"data"}}}
		});
	Value v2 = v1;

	EXPECT_EQ(v1, v2);

	map_set(v2, "key", std::string{ "modified" });

	EXPECT_EQ(map_get(v1, "key").as<std::string>(), "data");
	EXPECT_EQ(map_get(v2, "key").as<std::string>(), "modified");
}

TEST(FlexValueCOW, LargeStructureEfficiency)
{
	Value original = make_map();
	for (int i = 0; i < 1000; ++i) {
		map_set(original, "key" + std::to_string(i), Value{ int32_t{i} });
	}

	auto start = std::chrono::high_resolution_clock::now();
	Value copy = original;
	auto end = std::chrono::high_resolution_clock::now();

	auto duration
		= std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	EXPECT_LT(duration.count(), 100);

	map_set(copy, "key1", Value{ int32_t{999} });
	EXPECT_EQ(map_get(original, "key1").as<int32_t>(), 1);
}

// ============================================================
// ЧАСТЬ 11: PATH TRACKING В ИСКЛЮЧЕНИЯХ
// ============================================================
TEST(FlexValueExceptions, KeyNotFoundWithPath)
{
	Value m = make_map();
	try {
		map_get(m, "missing");
		FAIL() << "Should have thrown KeyNotFoundException";
	}
	catch (const KeyNotFoundException& e) {
		EXPECT_TRUE(e.path.find("missing") != std::string::npos);
	}
}

TEST(FlexValueExceptions, TypeMismatchWithPath)
{
	Value v{ int32_t{42} };
	try {
		v.as<std::string>("/root/field");
		FAIL() << "Should have thrown TypeMismatchException";
	}
	catch (const TypeMismatchException& e) {
		EXPECT_TRUE(e.path.find("field") != std::string::npos);
		EXPECT_TRUE(
			std::string(e.what()).find("Type mismatch") != std::string::npos
		);
	}
}

TEST(FlexValueExceptions, NotContainerException)
{
	Value v{ int32_t{42} };
	try {
		array_push(v, int32_t{ 1 });
		FAIL() << "Should have thrown NotContainerException";
	}
	catch (const NotContainerException& e) {
		EXPECT_TRUE(std::string(e.what()).find("Array") != std::string::npos);
	}
}

TEST(FlexValueExceptions, ArrayIndexOutOfBounds)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });

	try {
		array_at(arr, 5);
		FAIL() << "Should have thrown out_of_range";
	}
	catch (const std::out_of_range& e) {
		EXPECT_TRUE(std::string(e.what()).find("bounds") != std::string::npos);
	}
}

// ============================================================
// ЧАСТЬ 12: BUILDER PATTERN
// ============================================================
TEST(FlexValueBuilder, SimpleMap)
{
	ValueBuilder builder;
	Value v = builder.set("name", "Test")
		.set("age", int32_t{ 42 })
		.set("active", true)
		.build();

	EXPECT_EQ(map_get(v, "name").as<std::string>(), "Test");
	EXPECT_EQ(map_get(v, "age").as<int32_t>(), 42);
	EXPECT_EQ(map_get(v, "active").as<bool>(), true);
}

TEST(FlexValueBuilder, Array)
{
	ValueBuilder builder(make_array());

	Value v
		= builder.push(int32_t{ 1 }).push(int32_t{ 2 }).push(int32_t{ 3 }).build();

	EXPECT_EQ(array_size(v), 3);
}

TEST(FlexValueBuilder, NestedStructure)
{
	ValueBuilder builder;
	builder.set("version", int32_t{ 1 })
		.set("name", "MyApp")
		.enter("config")
		.set("timeout", int32_t{ 5000 })
		.set("retries", int32_t{ 3 })
		.exit()
		.set("enabled", true);

	Value v = builder.build();
	EXPECT_EQ(map_get(v, "version").as<int32_t>(), 1);
	const auto& cfg = map_get(v, "config");
	EXPECT_EQ(map_get(cfg, "timeout").as<int32_t>(), 5000);
}

// ============================================================
// ЧАСТЬ 13: TRAVERSAL И FIND
// ============================================================
TEST(FlexValueTraversal, SimpleTraverse)
{
	Value root = make_map({
		{"a", Value{int32_t{1}}                   },
		{"b", make_map({{"c", Value{int32_t{2}}}})},
		{"d", make_array()                        }
		});

	std::vector<std::string> paths;
	traverse(root, [&](const Value&, const std::string& path) {
		paths.push_back(path);
		});

	EXPECT_GT(paths.size(), 3);
	EXPECT_TRUE(std::any_of(paths.begin(), paths.end(), [](const auto& p) {
		return p.find("a") != std::string::npos;
		}));
}

TEST(FlexValueFind, FindByType)
{
	Value root = make_map({
		{"int1", Value{int32_t{10}}        },
		{"str1", Value{std::string{"text"}}},
		{"int2", Value{int32_t{20}}        },
		{"arr",  make_array()              }
		});

	auto results
		= find_all(root, [](const Value& v) { return v.is<int32_t>(); });

	EXPECT_EQ(results.size(), 2);
}

TEST(FlexValueFind, FindByValue)
{
	Value arr = make_array();
	array_push(
		arr,
		make_map({
			{"id",   Value{int32_t{1}}          },
			{"name", Value{std::string{"Alice"}}}
			})
	);
	array_push(
		arr,
		make_map({
			{"id",   Value{int32_t{2}}        },
			{"name", Value{std::string{"Bob"}}}
			})
	);
	array_push(
		arr,
		make_map({
			{"id",   Value{int32_t{3}}          },
			{"name", Value{std::string{"Alice"}}}
			})
	);

	auto results = find_all(arr, [](const Value& v) {
		return v.is<std::string>() && v.as<std::string>() == "Alice";
		});

	EXPECT_EQ(results.size(), 2);
}

// ============================================================
// ЧАСТЬ 14: MAP INDEXING
// ============================================================
TEST(FlexValueMapIndex, HasKeyCheck)
{
	Value m = make_map({
		{"key1", Value{int32_t{10}}}
		});

	EXPECT_TRUE(map_has_key(m, "key1"));
	EXPECT_FALSE(map_has_key(m, "missing"));
}

TEST(FlexValueMapIndex, QuickLookup)
{
	Value m = make_map();
	for (int i = 0; i < 100; ++i) {
		map_set(m, "key" + std::to_string(i), Value{ int32_t{i} });
	}

	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 100; ++i) {
		[[maybe_unused]] auto v = map_get(m, "key" + std::to_string(i));
	}
	auto end = std::chrono::high_resolution_clock::now();

	auto duration
		= std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	EXPECT_LT(duration.count(), 10);
}

// ============================================================
// ЧАСТЬ 15: SCHEMA VALIDATION
// ============================================================
TEST(FlexValueSchema, ValidateSimpleMap)
{
	auto schema = std::make_unique<Schema>();
	schema->type = "map";
	schema->properties["name"] = std::make_unique<Schema>();
	schema->properties["name"]->type = "string";
	schema->properties["age"] = std::make_unique<Schema>();
	schema->properties["age"]->type = "int32";

	Value valid = make_map({
		{"name", Value{std::string{"John"}}},
		{"age",  Value{int32_t{30}}        }
		});

	std::string error;
	EXPECT_TRUE(schema->validate(valid, error));
}

TEST(FlexValueSchema, ValidateArray)
{
	auto schema = std::make_unique<Schema>();
	schema->type = "array";
	schema->items = std::make_unique<Schema>();
	schema->items->type = "int32";

	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });
	array_push(arr, int32_t{ 3 });

	std::string error;
	EXPECT_TRUE(schema->validate(arr, error));
}

TEST(FlexValueSchema, InvalidTypeDetection)
{
	auto schema = std::make_unique<Schema>();
	schema->type = "int32";

	Value invalid{ std::string{"not an int"} };

	std::string error;
	EXPECT_FALSE(schema->validate(invalid, error));
	EXPECT_TRUE(error.find("Expected int32") != std::string::npos);
}

TEST(FlexValueSchema, RequiredField)
{
	auto schema = std::make_unique<Schema>();
	schema->type = "map";
	schema->required = true;

	Value null_val = make_null();

	std::string error;
	EXPECT_FALSE(schema->validate(null_val, error));
	EXPECT_TRUE(error.find("Required") != std::string::npos);
}

// ============================================================
// ЧАСТЬ 16: ИНТРОСПЕКЦИЯ
// ============================================================
TEST(FlexValueIntrospection, MapKeys)
{
	Value m = make_map({
		{"alpha", Value{int32_t{1}}},
		{"beta",  Value{int32_t{2}}},
		{"gamma", Value{int32_t{3}}}
		});

	auto keys = ValueIntrospector::map_keys(m);
	EXPECT_EQ(keys.size(), 3);
	EXPECT_TRUE(std::find(keys.begin(), keys.end(), "alpha") != keys.end());
}

TEST(FlexValueIntrospection, ArrayLength)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });

	EXPECT_EQ(ValueIntrospector::array_length(arr), 2);
}

TEST(FlexValueIntrospection, TypeName)
{
	EXPECT_EQ(ValueIntrospector::type_name(Value{ int32_t{42} }), "int32");
	EXPECT_EQ(
		ValueIntrospector::type_name(Value{ std::string{"test"} }), "string"
	);
	EXPECT_EQ(ValueIntrospector::type_name(Value{ true }), "bool");
	EXPECT_EQ(ValueIntrospector::type_name(make_map()), "map");
	EXPECT_EQ(ValueIntrospector::type_name(make_array()), "array");
}

// ============================================================
// ЧАСТЬ 17: ARRAY SAFE ACCESS
// ============================================================
TEST(FlexValueArrayAccess, SafeAt)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 10 });
	array_push(arr, int32_t{ 20 });
	array_push(arr, int32_t{ 30 });

	EXPECT_EQ(array_at(arr, 0).as<int32_t>(), 10);
	EXPECT_EQ(array_at(arr, 1).as<int32_t>(), 20);
	EXPECT_EQ(array_at(arr, 2).as<int32_t>(), 30);
}

TEST(FlexValueArrayAccess, ConstAt)
{
	const Value arr = make_array();
	Value& arr_mut = const_cast<Value&>(arr);
	array_push(arr_mut, int32_t{ 42 });

	EXPECT_EQ(array_at(arr, 0).as<int32_t>(), 42);
}

// ============================================================
// ЧАСТЬ 18: ГРАНИЧНЫЕ СЛУЧАИ
// ============================================================
TEST(FlexValueEdgeCases, EmptyContainers)
{
	Value empty_arr = make_array();
	Value empty_map = make_map();

	EXPECT_EQ(array_size(empty_arr), 0);
	EXPECT_EQ(ValueIntrospector::map_keys(empty_map).size(), 0);
}

TEST(FlexValueEdgeCases, NullValue)
{
	Value null_val = make_null();
	EXPECT_TRUE(null_val.is_null());
	EXPECT_EQ(ValueIntrospector::type_name(null_val), "null");
}

TEST(FlexValueEdgeCases, StringWithSpecialChars)
{
	Value v{ std::string{"\n\t\r\\\"quoted\""} };
	EXPECT_EQ(v.as<std::string>(), "\n\t\r\\\"quoted\"");
}

TEST(FlexValueEdgeCases, LargeNumbers)
{
	Value max_i64{ int64_t{9223372036854775807LL} };
	Value min_i64{ int64_t{-9223372036854775807LL} };

	EXPECT_EQ(max_i64.as<int64_t>(), 9223372036854775807LL);
	EXPECT_EQ(min_i64.as<int64_t>(), -9223372036854775807LL);
}

// ============================================================
// ЧАСТЬ 19: СЛОЖНЫЕ СЦЕНАРИИ
// ============================================================
TEST(FlexValueComplex, DVB_PMT_WithBuilderAndValidation)
{
	ValueBuilder pmt_builder;
	pmt_builder.set("table_id", uint32_t{ 0x02 })
		.set("program_number", uint32_t{ 100 })
		.set("pcr_pid", uint32_t{ 256 })
		.enter("descriptors")
		.exit();

	Value pmt = pmt_builder.build();

	auto numeric_values = find_all(pmt, [](const Value& v) {
		return v.is<uint32_t>() || v.is<int32_t>();
		});

	EXPECT_GE(numeric_values.size(), 3);
}

TEST(FlexValueComplex, Deep_Nested_Query)
{
	Value root = make_map();
	for (int i = 0; i < 5; ++i) {
		Value level = make_map();
		for (int j = 0; j < 10; ++j) {
			Value item = make_map({
				{"id",     Value{int32_t{i * 10 + j}}},
				{"active", Value{j % 2 == 0}         },
				{"tags",   make_array()              }
				});
			Value tags_arr = make_array();
			array_push(tags_arr, std::string{ "tag1" });
			array_push(tags_arr, std::string{ "tag2" });
			map_set(item, "tags", tags_arr);

			map_set(level, "item" + std::to_string(j), item);
		}
		map_set(root, "level" + std::to_string(i), level);
	}

	auto active = find_all(root, [](const Value& v) {
		return v.is<bool>() && v.as<bool>();
		});

	EXPECT_EQ(active.size(), 25);
}

TEST(FlexValueComplex, MapKeyIterationAndModification)
{
	Value m = make_map();
	for (int i = 0; i < 50; ++i) {
		map_set(m, "k" + std::to_string(i), Value{ int32_t{i} });
	}

	auto keys = ValueIntrospector::map_keys(m);
	EXPECT_EQ(keys.size(), 50);

	for (const auto& key : keys) {
		EXPECT_NO_THROW(map_get(m, key));
	}
}

// ============================================================
// НОВЫЕ ТЕСТЫ: Покрытие пробелов
// ============================================================

// ============================================================
// ТЕСТ 1: Функция wrap() - было пробелом
// ============================================================
TEST(FlexValueWrap, WrapLiterals)
{
	// wrap с const char*
	Value v1 = wrap("hello");
	EXPECT_TRUE(v1.is<std::string>());
	EXPECT_EQ(v1.as<std::string>(), "hello");

	// wrap со строкой
	std::string str{ "world" };
	Value v2 = wrap(str);
	EXPECT_TRUE(v2.is<std::string>());
	EXPECT_EQ(v2.as<std::string>(), "world");

	// wrap с int32
	Value v3 = wrap(int32_t{ 42 });
	EXPECT_TRUE(v3.is<int32_t>());
	EXPECT_EQ(v3.as<int32_t>(), 42);

	// wrap с bool
	Value v4 = wrap(true);
	EXPECT_TRUE(v4.is<bool>());
	EXPECT_EQ(v4.as<bool>(), true);

	// wrap с Fraction
	Fraction frac{ 3, 4 };
	Value v5 = wrap(frac);
	EXPECT_TRUE(v5.is<Fraction>());
	EXPECT_EQ(v5.as<Fraction>().num, 3);
}

// ============================================================
// ТЕСТ 2: Fraction с нулевым denominator - ИСПРАВЛЕНО!
// ============================================================
TEST(FlexValueFraction, ZeroDenominatorThrows)
{
	EXPECT_THROW(make_fraction(1, 0), InvalidArgumentException);
	// EXPECT_THROW(Fraction{1, 0}, InvalidArgumentException);
}

TEST(FlexValueFraction, ValidFraction)
{
	// Через make_fraction
	Value frac1 = make_fraction(5, 3);
	EXPECT_EQ(frac1.as<Fraction>().num, 5);
	EXPECT_EQ(frac1.as<Fraction>().den, 3);

	// Через конструктор
	Fraction f{ 7, 2 };
	Value frac2{ f };
	EXPECT_EQ(frac2.as<Fraction>().num, 7);
	EXPECT_EQ(frac2.as<Fraction>().den, 2);
}

// ============================================================
// ТЕСТ 3: EnumValue и FlagsValue конструкторы
// ============================================================
TEST(FlexValueEnum, EnumWithTypeName)
{
	Value e1 = make_enum(5, "VideoCodec");
	EXPECT_EQ(e1.as<EnumValue>().value, 5);
	EXPECT_EQ(e1.as<EnumValue>().type_name, "VideoCodec");

	Value e2 = make_enum(10);
	EXPECT_EQ(e2.as<EnumValue>().value, 10);
	EXPECT_EQ(e2.as<EnumValue>().type_name, "");
}

TEST(FlexValueFlags, FlagsWithTypeName)
{
	Value f1 = make_flags(0xFF, "AccessMask");
	EXPECT_EQ(f1.as<FlagsValue>().value, 0xFF);
	EXPECT_EQ(f1.as<FlagsValue>().type_name, "AccessMask");

	Value f2 = make_flags(0x0F);
	EXPECT_EQ(f2.as<FlagsValue>().value, 0x0F);
	EXPECT_EQ(f2.as<FlagsValue>().type_name, "");
}

// ============================================================
// ТЕСТ 4: map_has_key на некорректных типах - было пробелом
// ============================================================
TEST(FlexValueMapHasKey, InvalidTypes)
{
	Value v_int{ int32_t{42} };
	Value v_str{ std::string{"test"} };
	Value v_arr = make_array();

	EXPECT_FALSE(map_has_key(v_int, "key"));
	EXPECT_FALSE(map_has_key(v_str, "key"));
	EXPECT_FALSE(map_has_key(v_arr, "key"));
}

TEST(FlexValueMapHasKey, ValidMap)
{
	Value m = make_map();
	map_set(m, "existing", int32_t{ 10 });

	EXPECT_TRUE(map_has_key(m, "existing"));
	EXPECT_FALSE(map_has_key(m, "missing"));
}

// ============================================================
// ТЕСТ 5: array_at() граничные случаи - было пробелом
// ============================================================
TEST(FlexValueArrayAt, EmptyArrayAccess)
{
	Value arr = make_array();
	EXPECT_THROW(array_at(arr, 0), std::out_of_range);
}

TEST(FlexValueArrayAt, OutOfBoundsAccess)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });

	EXPECT_NO_THROW(array_at(arr, 0));
	EXPECT_NO_THROW(array_at(arr, 1));
	EXPECT_THROW(array_at(arr, 2), std::out_of_range);
	EXPECT_THROW(array_at(arr, 100), std::out_of_range);
}

TEST(FlexValueArrayAt, ConstArrayAt)
{
	const Value arr = make_array();
	Value& arr_mut = const_cast<Value&>(arr);
	array_push(arr_mut, int32_t{ 99 });

	EXPECT_EQ(array_at(arr, 0).as<int32_t>(), 99);
	EXPECT_THROW(array_at(arr, 1), std::out_of_range);
}

// ============================================================
// ТЕСТ 6: Цепочки операций map_get → map_set - было пробелом
// ============================================================
TEST(FlexValueChaining, GetSetChain)
{
	Value outer = make_map();
	Value inner = make_map();
	map_set(inner, "value", int32_t{ 42 });
	map_set(outer, "inner", inner);

	auto& retrieved = map_get(outer, "inner");
	EXPECT_EQ(map_get(retrieved, "value").as<int32_t>(), 42);

	map_set(retrieved, "value", int32_t{ 100 });
	EXPECT_EQ(map_get(map_get(outer, "inner"), "value").as<int32_t>(), 100);
}

// ============================================================
// ТЕСТ 7: COW при исключениях - было пробелом
// ============================================================
TEST(FlexValueCOWExceptions, ExceptionSafety)
{
	Array arr;
	arr.items.push_back(Value{ int32_t{1} });
	CowBox<Array> box1{ std::move(arr) };

	CowBox<Array> box2 = box1;
	EXPECT_EQ(box1.ref_count(), 2);

	try {
		throw std::runtime_error("test");
	}
	catch (...) {
		// После исключения box2 должен быть всё ещё валидным
		EXPECT_EQ(box2.ref_count(), 2);
	}

	// Проверяем целостность
	EXPECT_EQ(box1.get().items[0].as<int32_t>(), 1);
	EXPECT_EQ(box2.get().items[0].as<int32_t>(), 1);
}

// ============================================================
// ТЕСТ 8: last_access_path отслеживание - было пробелом
// ============================================================
TEST(FlexValueLastAccessPath, PathTracking)
{
	Value m = make_map();
	map_set(m, "key1", int32_t{ 10 });
	map_set(m, "key2", int32_t{ 20 });

	const auto& v1 = map_get(m, "key1");
	UNREFERENCED(v1);
	EXPECT_TRUE(m.last_access_path.find("key1") != std::string::npos);

	const auto& v2 = map_get(m, "key2");
	UNREFERENCED(v2);
	EXPECT_TRUE(m.last_access_path.find("key2") != std::string::npos);
}

// ============================================================
// ТЕСТ 9: Path в исключениях для вложенных структур - было пробелом
// ============================================================
TEST(FlexValueExceptionPath, NestedPath)
{
	Value root = make_map();
	Value nested = make_map();
	map_set(nested, "value", int32_t{ 42 });
	map_set(root, "nested", nested);

	try {
		map_get(map_get(root, "nested"), "missing");
		FAIL() << "Should have thrown";
	}
	catch (const KeyNotFoundException& e) {
		EXPECT_TRUE(std::string(e.what()).find("missing") != std::string::npos);
	}
}

// ============================================================
// ТЕСТ 10: Производительность traverse с большой структурой - было пробелом
// ============================================================
TEST(FlexValueTraversalPerf, LargeStructure)
{
	Value root = make_map();

	// Создаём большую вложенную структуру
	for (int i = 0; i < 10; ++i) {
		Value level1 = make_map();
		for (int j = 0; j < 10; ++j) {
			Value level2 = make_map();
			map_set(level2, "value", int32_t{ i * 10 + j });
			map_set(level1, "item" + std::to_string(j), level2);
		}
		map_set(root, "level" + std::to_string(i), level1);
	}

	auto start = std::chrono::high_resolution_clock::now();

	int count = 0;
	traverse(root, [&](const Value&, const std::string&) { count++; });

	auto end = std::chrono::high_resolution_clock::now();
	auto duration
		= std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_GT(count, 100); // Проверяем, что traverse прошёлся по узлам
	EXPECT_LT(duration.count(), 50); // Должно быть быстро
}

// ============================================================
// ТЕСТ 11: Удаление опасного Map.index - проверяем всё работает
// ============================================================
TEST(FlexValueMapSafety, COWWithModifications)
{
	Value v1 = make_map();
	map_set(v1, "key1", int32_t{ 10 });
	map_set(v1, "key2", int32_t{ 20 });
	map_set(v1, "key3", int32_t{ 30 });

	Value v2 = v1;

	// Модификация v2
	map_set(v2, "key1", int32_t{ 100 });

	// v1 должна остаться неизменной благодаря COW
	EXPECT_EQ(map_get(v1, "key1").as<int32_t>(), 10);
	EXPECT_EQ(map_get(v2, "key1").as<int32_t>(), 100);

	// Добавляем новые ключи
	map_set(v2, "key4", int32_t{ 40 });

	EXPECT_THROW(map_get(v1, "key4"), KeyNotFoundException);
	EXPECT_EQ(map_get(v2, "key4").as<int32_t>(), 40);
}

// ============================================================
// ТЕСТ 12: map_set_strict с вложенными структурами - было пробелом
// ============================================================
TEST(FlexValueMapSetStrict, NestedStructures)
{
	Value outer = make_map();
	Value inner = make_map();
	map_set(inner, "value", int32_t{ 42 });
	map_set(outer, "inner", inner);

	auto& retrieved = map_get(outer, "inner");

	EXPECT_TRUE(map_set_strict(retrieved, "value", int32_t{ 100 }));
	EXPECT_EQ(map_get(retrieved, "value").as<int32_t>(), 100);

	EXPECT_FALSE(map_set_strict(retrieved, "missing", int32_t{ 50 }));
}

// ============================================================
// ТЕСТ 13: ValueBuilder.get_root() - была добавлена новая функция
// ============================================================
TEST(FlexValueBuilder, GetRoot)
{
	ValueBuilder builder;
	builder.set("key", int32_t{ 42 });

	const auto& root = builder.get_root();
	EXPECT_EQ(map_get(root, "key").as<int32_t>(), 42);
}

// ============================================================
// ТЕСТ 14: Обработка пустой path в операциях
// ============================================================
TEST(FlexValueEmptyPath, Operations)
{
	Value m = make_map();
	map_set(m, "key", int32_t{ 10 }, ""); // пустая parent_path

	EXPECT_EQ(map_get(m, "key", "").as<int32_t>(), 10);
}

// ============================================================
// ТЕСТ 15: InvalidArgumentException
// ============================================================
TEST(FlexValueExceptions, InvalidArgumentException)
{
	EXPECT_THROW(make_fraction(1, 0), InvalidArgumentException);

	try {
		make_fraction(5, 0);
		FAIL() << "Should have thrown";
	}
	catch (const InvalidArgumentException& e) {
		EXPECT_TRUE(
			std::string(e.what()).find("denominator") != std::string::npos
		);
	}
}

// ============================================================
// ТЕСТ 16: Round-trip to_string и обратно (частичная проверка)
// ============================================================
TEST(FlexValueSerialization, ToStringFormats)
{
	// Проверяем форматирование различных типов
	EXPECT_EQ(to_string(Value{ true }), "true");
	EXPECT_EQ(to_string(Value{ false }), "false");
	EXPECT_EQ(to_string(Value{ int32_t{42} }), "42");
	EXPECT_EQ(to_string(Value{ std::string{"test"} }), "\"test\"");

	Value frac = make_fraction(3, 4);
	EXPECT_EQ(to_string(frac), "3/4");

	Value e = make_enum(5, "MyEnum");
	std::string es = to_string(e);
	EXPECT_NE(es.find("enum"), std::string::npos);
	EXPECT_NE(es.find("5"), std::string::npos);
}

// ============================================================
// ТЕСТ 17: ValueIntrospector согласованность
// ============================================================
TEST(FlexValueIntrospection, Consistency)
{
	Value arr = make_array();
	array_push(arr, int32_t{ 1 });
	array_push(arr, int32_t{ 2 });
	array_push(arr, int32_t{ 3 });

	// array_length из интроспектора должен совпадать с array_size
	EXPECT_EQ(ValueIntrospector::array_length(arr), array_size(arr));
	EXPECT_EQ(ValueIntrospector::array_length(arr), 3);
}

// ============================================================
// ТЕСТ 18: Граничный случай - множественные exit() вызовы
// ============================================================
TEST(FlexValueBuilder, MultipleExit)
{
	ValueBuilder builder;
	builder.set("a", int32_t{ 1 })
		.enter("b")
		.set("c", int32_t{ 2 })
		.exit()
		.exit()  // Лишний exit
		.exit(); // И ещё один

	Value v = builder.build();
	EXPECT_EQ(map_get(v, "a").as<int32_t>(), 1);
	EXPECT_EQ(map_get(map_get(v, "b"), "c").as<int32_t>(), 2);
}

// ============================================================
// ТЕСТ 19: Array с максимальным размером
// ============================================================
TEST(FlexValueArray, LargeArray)
{
	Value arr = make_array();

	// Добавляем 1000 элементов
	for (int i = 0; i < 1000; ++i) {
		array_push(arr, int32_t{ i });
	}

	EXPECT_EQ(array_size(arr), 1000);
	EXPECT_EQ(array_at(arr, 0).as<int32_t>(), 0);
	EXPECT_EQ(array_at(arr, 999).as<int32_t>(), 999);
}

/////////ААААААААААААААААА
// АААААААААААААААА
// ААААААААААААА
// АААААААААААААА
// ============================================================
// ТЕСТ 20 (ИСПРАВЛЕННЫЙ): Глубокая вложенность
// ============================================================
TEST(FlexValueDeepNesting, VeryDeep)
{
	// Используем ValueBuilder вместо манипулирования Value напрямую
	// Это намного безопаснее и эффективнее
	ValueBuilder builder;

	// Вместо цепочки map_get используем enter/exit для навигации
	builder.enter("level0")
		.enter("level1")
		.enter("level2")
		.enter("level3")
		.enter("level4")
		.enter("level5")
		.enter("level6")
		.enter("level7")
		.enter("level8")
		.enter("level9")
		.set("value", int32_t{ 42 })
		.exit()
		.exit()
		.exit()
		.exit()
		.exit()
		.exit()
		.exit()
		.exit()
		.exit()
		.exit();

	Value root = builder.build();

	// Проверяем через Builder's path tracking, не через последовательные
	// map_get
	const auto& level0 = map_get(root, "level0");
	const auto& level1 = map_get(level0, "level1");
	const auto& level2 = map_get(level1, "level2");
	const auto& level3 = map_get(level2, "level3");
	const auto& level4 = map_get(level3, "level4");
	const auto& level5 = map_get(level4, "level5");
	const auto& level6 = map_get(level5, "level6");
	const auto& level7 = map_get(level6, "level7");
	const auto& level8 = map_get(level7, "level8");
	const auto& level9 = map_get(level8, "level9");

	EXPECT_EQ(map_get(level9, "value").as<int32_t>(), 42);
}

// ============================================================
// ДОПОЛНИТЕЛЬНЫЙ ТЕСТ: Простая вложенность (лучше для stress-теста)
// ============================================================
TEST(FlexValueDeepNesting, ModerateNesting)
{
	// 5 уровней вложенности - хороший компромисс
	ValueBuilder builder;
	builder.enter("a")
		.enter("b")
		.enter("c")
		.enter("d")
		.enter("e")
		.set("data", int32_t{ 123 })
		.exit()
		.exit()
		.exit()
		.exit()
		.exit();

	Value root = builder.build();

	const auto& a = map_get(root, "a");
	const auto& b = map_get(a, "b");
	const auto& c = map_get(b, "c");
	const auto& d = map_get(c, "d");
	const auto& e = map_get(d, "e");

	EXPECT_EQ(map_get(e, "data").as<int32_t>(), 123);
}

// ============================================================
// ТЕСТ: Широкая (не глубокая) структура - для массовых данных
// ============================================================
TEST(FlexValueDeepNesting, WideStructure)
{
	// Вместо глубокой вложенности, создаём структуру с множеством элементов на
	// одном уровне
	Value root = make_map();

	for (int i = 0; i < 100; ++i) {
		map_set(root, "key" + std::to_string(i), int32_t{ i });
	}

	// Проверяем несколько значений
	EXPECT_EQ(map_get(root, "key0").as<int32_t>(), 0);
	EXPECT_EQ(map_get(root, "key50").as<int32_t>(), 50);
	EXPECT_EQ(map_get(root, "key99").as<int32_t>(), 99);
}

// ============================================================
// АЛЬТЕРНАТИВА: Использование traverse для очень больших структур
// ============================================================
TEST(FlexValueDeepNesting, TraverseLargeStructure)
{
	Value root = make_map();

	// Создаём 3-уровневую структуру: категория -> подкатегория -> элемент
	for (int cat = 0; cat < 5; ++cat) {
		Value category = make_map();

		for (int subcat = 0; subcat < 5; ++subcat) {
			Value subcategory = make_map();

			for (int item = 0; item < 5; ++item) {
				Value element = make_map();
				map_set(element, "id", int32_t{ cat * 25 + subcat * 5 + item });
				map_set(
					element,
					"name",
					"Item_" + std::to_string(cat) + "_" + std::to_string(subcat)
					+ "_" + std::to_string(item)
				);

				map_set(subcategory, "item" + std::to_string(item), element);
			}

			map_set(category, "sub" + std::to_string(subcat), subcategory);
		}

		map_set(root, "cat" + std::to_string(cat), category);
	}

	// Подсчитываем элементы через traverse
	int element_count = 0;
	traverse(root, [&](const Value& v, const std::string& path) {
		if (v.is<int32_t>() && path.find("id") != std::string::npos) {
			element_count++;
		}
		});

	EXPECT_EQ(element_count, 5 * 5 * 5); // 125 элементов
}
