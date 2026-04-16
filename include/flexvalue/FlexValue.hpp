
// C++17, header-only
// Поддержка: BOOL, INT32, ..., STRING, FRACTION, CAPS, ENUM, FLAGS, STRUCTURE
#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace fxv
{
	namespace str_types {
		constexpr auto NULL_T = "null";
		constexpr auto BOOL_T = "bool";
		constexpr auto INT8_T = "int8";
		constexpr auto UINT8_T = "uint8";
		constexpr auto INT16_T = "int16";
		constexpr auto UINT16_T = "uint16";
		constexpr auto INT32_T = "int32";
		constexpr auto UINT32_T = "uint32";
		constexpr auto INT64_T = "int64";
		constexpr auto UINT64_T = "uint64";
		constexpr auto FLOAT_T = "float";
		constexpr auto DOUBLE_T = "double";
		constexpr auto STRING_T = "string";
		constexpr auto FRC_T = "fraction";
		constexpr auto ENUM_T = "enum";
		constexpr auto FLAG_T = "flags";
		constexpr auto ARRAY_T = "array";
		constexpr auto MAP_T = "map";
		constexpr auto UNK_T = "unknown";
	}


	class ValueException : public std::runtime_error
	{
	public:
		std::string type_info;

		ValueException(
			const std::string& msg,
			const std::string& ti = ""
		) :
			std::runtime_error(format_message(msg, ti)),
			type_info(ti)
		{}

	private:
		static std::string format_message(
			const std::string& msg,
			const std::string& type_info
		)
		{
			std::string result = msg;
			if (!type_info.empty()) result += " (type: " + type_info + ")";
			return result;
		}
	};

	class KeyNotFoundException : public ValueException
	{
	public:
		KeyNotFoundException(const std::string& key) :
			ValueException("Key not found: " + key, "Map")
		{}
	};

	class TypeMismatchException : public ValueException
	{
	public:
		TypeMismatchException(
			const std::string& expected,
			const std::string& actual
		) :
			ValueException(
				"Type mismatch: expected " + expected + ", got " + actual
			)
		{}
	};

	class NotContainerException : public ValueException
	{
	public:
		NotContainerException(
			const std::string& container_type
		) :
			ValueException("Value is not a " + container_type)
		{}
	};

	class InvalidArgumentException : public ValueException
	{
	public:
		InvalidArgumentException(
			const std::string& msg
		) :
			ValueException(msg)
		{}
	};

	// ============================================================
	// Типы
	// ============================================================
	struct Value; // объявляем, но определение будет выше первого использования в
				  // контейнерах

	struct Fraction
	{
		int32_t num = 0;
		int32_t den = 1;

		Fraction() = default;

		Fraction(int32_t n, int32_t d) :
			num(n),
			den(d)
		{
			if (d == 0) {
				throw InvalidArgumentException("Fraction denominator cannot be zero"
				);
			}
		}

		bool operator==(const Fraction& o) const noexcept
		{
			return num == o.num && den == o.den;
		}

		bool operator!=(const Fraction& o) const noexcept { return !(*this == o); }
	};

	struct EnumValue
	{
		int32_t value = 0;
		std::string type_name;

		EnumValue() = default;

		EnumValue(int32_t v, std::string t = "") :
			value(v),
			type_name(std::move(t))
		{}

		bool operator==(const EnumValue& o) const noexcept
		{
			return value == o.value && type_name == o.type_name;
		}

		bool operator!=(const EnumValue& o) const noexcept { return !(*this == o); }
	};

	struct FlagsValue
	{
		uint32_t value = 0;
		std::string type_name;

		FlagsValue() = default;

		FlagsValue(uint32_t v, std::string t = "") :
			value(v),
			type_name(std::move(t))
		{}

		bool operator==(const FlagsValue& o) const noexcept
		{
			return value == o.value && type_name == o.type_name;
		}

		bool operator!=(const FlagsValue& o) const noexcept
		{
			return !(*this == o);
		}
	};

	// Copy-on-write Box
	template<typename T>
	class CowBox
	{
		std::shared_ptr<T> ptr_;

	public:
		CowBox() :
			ptr_(std::make_shared<T>())
		{}

		explicit CowBox(T v) :
			ptr_(std::make_shared<T>(std::move(v)))
		{}

		CowBox(const CowBox& other) :
			ptr_(other.ptr_)
		{}

		CowBox(CowBox&&) noexcept = default;

		CowBox& operator=(const CowBox& other)
		{
			if (this != &other) ptr_ = other.ptr_;
			return *this;
		}

		CowBox& operator=(CowBox&&) noexcept = default;

		T& get()
		{
			if (ptr_.use_count() > 1) {
				ptr_ = std::make_shared<T>(*ptr_);
			}
			return *ptr_;
		}

		const T& get() const { return *ptr_; }

		T* operator->() { return &get(); }

		const T* operator->() const { return &get(); }

		friend bool operator==(const CowBox& a, const CowBox& b) noexcept
		{
			if (!a.ptr_ || !b.ptr_) return a.ptr_ == b.ptr_;
			return *a.ptr_ == *b.ptr_;
		}

		friend bool operator!=(const CowBox& a, const CowBox& b) noexcept
		{
			return !(a == b);
		}

		long ref_count() const { return ptr_.use_count(); }
	};

	struct Array;
	struct Map;

	// ============================================================
	// Main Value type with COW
	// ============================================================
	struct Value
	{
		std::variant<
			std::monostate,
			bool,
			int8_t,
			uint8_t,
			int16_t,
			uint16_t,
			int32_t,
			uint32_t,
			int64_t,
			uint64_t,
			float,
			double,
			std::string,
			Fraction,
			EnumValue,
			FlagsValue,
			CowBox<Array>,
			CowBox<Map>>
			data;

		Value() :
			data(std::monostate{})
		{}

		explicit Value(bool v) :
			data(v)
		{}

		explicit Value(int8_t v) :
			data(v)
		{}

		explicit Value(uint8_t v) :
			data(v)
		{}

		explicit Value(int16_t v) :
			data(v)
		{}

		explicit Value(uint16_t v) :
			data(v)
		{}

		explicit Value(int32_t v) :
			data(v)
		{}

		explicit Value(uint32_t v) :
			data(v)
		{}

		explicit Value(int64_t v) :
			data(v)
		{}

		explicit Value(uint64_t v) :
			data(v)
		{}

		explicit Value(float v) :
			data(v)
		{}

		explicit Value(double v) :
			data(v)
		{}

		explicit Value(std::string v) :
			data(std::move(v))
		{}

		explicit Value(const char* v) :
			data(std::string(v))
		{}

		explicit Value(Fraction v) :
			data(v)
		{}

		explicit Value(EnumValue v) :
			data(std::move(v))
		{}

		explicit Value(FlagsValue v) :
			data(std::move(v))
		{}

		explicit Value(CowBox<Array> v) :
			data(std::move(v))
		{}

		explicit Value(CowBox<Map> v) :
			data(std::move(v))
		{}

		template<class T>
		bool is() const
		{
			return std::holds_alternative<T>(data);
		}

		template<class T>
		const T& as() const
		{
			if (!std::holds_alternative<T>(data)) {
				throw TypeMismatchException(typeid(T).name(), typeid_name());
			}
			return std::get<T>(data);
		}

		template<class T>
		T& as()
		{
			if (!std::holds_alternative<T>(data)) {
				throw TypeMismatchException(typeid(T).name(), typeid_name());
			}
			return std::get<T>(data);
		}

		bool is_null() const
		{
			return std::holds_alternative<std::monostate>(data);
		}

		std::string typeid_name() const
		{
			return std::visit(
				[](auto const& arg) -> std::string {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, std::monostate>) return str_types::NULL_T;
					else if constexpr (std::is_same_v<T, bool>) return str_types::BOOL_T;
					else if constexpr (std::is_same_v<T, int16_t>) return str_types::INT8_T;
					else if constexpr (std::is_same_v<T, uint16_t>) return str_types::UINT8_T;
					else if constexpr (std::is_same_v<T, int16_t>) return str_types::INT16_T;
					else if constexpr (std::is_same_v<T, uint16_t>) return str_types::UINT16_T;
					else if constexpr (std::is_same_v<T, int32_t>) return str_types::INT32_T;
					else if constexpr (std::is_same_v<T, uint32_t>) return str_types::UINT32_T;
					else if constexpr (std::is_same_v<T, int64_t>) return str_types::INT64_T;
					else if constexpr (std::is_same_v<T, uint64_t>) return str_types::UINT64_T;
					else if constexpr (std::is_same_v<T, float>) return str_types::FLOAT_T;
					else if constexpr (std::is_same_v<T, double>) return str_types::DOUBLE_T;
					else if constexpr (std::is_same_v<T, std::string>)
						return str_types::STRING_T;
					else if constexpr (std::is_same_v<T, Fraction>)
						return str_types::FRC_T;
					else if constexpr (std::is_same_v<T, EnumValue>) return str_types::ENUM_T;
					else if constexpr (std::is_same_v<T, FlagsValue>)
						return str_types::FLAG_T;
					else if constexpr (std::is_same_v<T, CowBox<Array>>)
						return str_types::ARRAY_T;
					else if constexpr (std::is_same_v<T, CowBox<Map>>) return str_types::MAP_T;
					else return str_types::UNK_T;
				},
				data
					);
		}

		bool operator==(const Value& other) const noexcept
		{
			return data == other.data;
		}

		bool operator!=(const Value& other) const noexcept
		{
			return !(*this == other);
		}
	};

	// Теперь, когда Value ОПРЕДЕЛЕН, можно определять контейнеры, которые его
	// содержат.
	struct Array
	{
		std::vector<Value> items;

		bool operator==(const Array& o) const noexcept { return items == o.items; }

		bool operator!=(const Array& o) const noexcept { return !((*this) == o); }
	};

	struct Map
	{
		std::map<std::string, Value> fields;

		bool operator==(const Map& o) const noexcept { return fields == o.fields; }

		bool operator!=(const Map& o) const noexcept { return !((*this) == o); }
	};

} // namespace fxv
