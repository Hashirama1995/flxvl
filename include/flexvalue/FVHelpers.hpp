#pragma once

#include <flexvalue/FlexValue.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace fxv
{
	// ============================================================
	// Фабрики и утилиты
	// ============================================================
	inline Value make_null() { return Value{}; }

	inline Value make_fraction(int32_t n, int32_t d)
	{
		return Value{
			Fraction{n, d}
		};
	}

	inline Value make_enum(int32_t v, std::string t = "")
	{
		return Value{
			EnumValue{v, std::move(t)}
		};
	}

	inline Value make_flags(uint32_t v, std::string t = "")
	{
		return Value{
			FlagsValue{v, std::move(t)}
		};
	}

	inline Value make_array(std::vector<Value> items = {})
	{
		Array a;
		a.items = std::move(items);
		return Value{ CowBox<Array>{std::move(a)} };
	}

	inline Value make_map(std::map<std::string, Value> f = {})
	{
		Map m;
		m.fields = std::move(f);
		return Value{ CowBox<Map>{std::move(m)} };
	}

	template<class U>
	inline Value wrap(U&& u)
	{
		using D = std::decay_t<U>;
		if constexpr (std::is_same_v<D, const char*>) {
			return Value{ std::string(u) };
		}
		else if constexpr (std::is_same_v<D, std::string>) {
			return Value{ std::forward<U>(u) };
		}
		else if constexpr (std::is_same_v<D, bool>) {
			return Value{ u };
		}
		else if constexpr (std::is_arithmetic_v<D>) {
			return Value{ std::forward<U>(u) };
		}
		else if constexpr (std::is_same_v<D, Fraction>) {
			return Value{ std::forward<U>(u) };
		}
		else if constexpr (std::is_same_v<D, EnumValue>) {
			return Value{ std::forward<U>(u) };
		}
		else if constexpr (std::is_same_v<D, FlagsValue>) {
			return Value{ std::forward<U>(u) };
		}
		else {
			return Value{ std::forward<U>(u) };
		}
	}

	inline Value& map_get(
		Value& val, const std::string& key
	)
	{
		if (!val.is<CowBox<Map>>()) {
			throw NotContainerException("Map");
		}
		auto& m = val.as<CowBox<Map>>().get();
		auto it = m.fields.find(key);
		if (it == m.fields.end()) {
			throw KeyNotFoundException(key);
		}
		return it->second;
	}

	inline const Value& map_get(
		const Value& val,
		const std::string& key
	)
	{
		if (!val.is<CowBox<Map>>()) {
			throw NotContainerException("Map");
		}
		const auto& fields = val.as<CowBox<Map>>().get().fields;
		auto it = fields.find(key);
		if (it == fields.end()) {
			throw KeyNotFoundException(key);
		}
		return it->second;
	}

	inline bool map_has_key(const Value& val, const std::string& key)
	{
		if (!val.is<CowBox<Map>>()) return false;
		return val.as<CowBox<Map>>().get().fields.count(key) > 0;
	}

	inline void map_set(
		Value& val,
		const std::string& key,
		Value v
	)
	{
		if (!val.is<CowBox<Map>>()) {
			throw NotContainerException("Map");
		}
		auto& m = val.as<CowBox<Map>>().get();
		m.fields[key] = std::move(v);
	}

	template<class U>
	inline void map_set(
		Value& val,
		const std::string& key,
		U&& u
	)
	{
		map_set(val, key, wrap(std::forward<U>(u)));
	}

	inline bool map_set_strict(
		Value& val,
		const std::string& key,
		Value v
	)
	{
		if (!val.is<CowBox<Map>>()) return false;
		auto& m = val.as<CowBox<Map>>().get();
		auto it = m.fields.find(key);
		if (it == m.fields.end()) return false;
		it->second = std::move(v);
		return true;
	}

	template<class U>
	inline bool map_set_strict(
		Value& val,
		const std::string& key,
		U&& u
	)
	{
		return map_set_strict(val, key, wrap(std::forward<U>(u)));
	}

	// ============================================================
	// Array access with path tracking
	// ============================================================
	inline void
		array_push(Value& val, Value item)
	{
		if (!val.is<CowBox<Array>>()) {
			throw NotContainerException("Array");
		}
		val.as<CowBox<Array>>().get().items.push_back(std::move(item));
	}

	template<class U>
	inline void array_push(Value& val, U&& u)
	{
		array_push(val, wrap(std::forward<U>(u)));
	}

	inline size_t array_size(const Value& val)
	{
		if (!val.is<CowBox<Array>>()) {
			throw NotContainerException("Array");
		}
		return val.as<CowBox<Array>>().get().items.size();
	}

	inline Value&
		array_at(Value& val, size_t idx)
	{
		if (!val.is<CowBox<Array>>()) {
			throw NotContainerException("Array");
		}
		auto& arr = val.as<CowBox<Array>>().get();
		if (idx >= arr.items.size()) {
			throw std::out_of_range(
				"Array index out of bounds: " + std::to_string(idx)
				+ " >= " + std::to_string(arr.items.size())
			);
		}
		return arr.items[idx];
	}

	inline const Value&
		array_at(const Value& val, size_t idx)
	{
		if (!val.is<CowBox<Array>>()) {
			throw NotContainerException("Array");
		}
		const auto& arr = val.as<CowBox<Array>>().get();
		if (idx >= arr.items.size()) {
			throw std::out_of_range("Array index out of bounds");
		}
		return arr.items[idx];
	}

	// ============================================================
	// Traversal и поиск
	// ============================================================
	using Visitor = std::function<void(const Value&, const std::string& path)>;
	using Predicate = std::function<bool(const Value&)>;

	inline void
		traverse(const Value& val, Visitor visitor, const std::string& path = "/")
	{
		visitor(val, path);
		if (val.is<CowBox<Array>>()) {
			const auto& arr = val.as<CowBox<Array>>().get().items;
			for (size_t i = 0; i < arr.size(); ++i) {
				traverse(arr[i], visitor, path + "[" + std::to_string(i) + "]");
			}
		}
		else if (val.is<CowBox<Map>>()) {
			const auto& m = val.as<CowBox<Map>>().get().fields;
			for (const auto& [k, v] : m) {
				traverse(v, visitor, path + "/" + k);
			}
		}
	}

	inline std::vector<std::pair<std::string, Value>>
		find_all(const Value& val, Predicate pred, const std::string& path = "/")
	{
		std::vector<std::pair<std::string, Value>> results;
		traverse(
			val,
			[&](const Value& v, const std::string& p) {
				if (pred(v)) results.push_back({ p, v });
			},
			path
				);
		return results;
	}

	// ============================================================
	// Сериализация в строку (для отладки)
	// ============================================================
	inline std::string to_string(const Value& val, size_t indent = 0)
	{
		std::string ind(indent, ' ');
		std::string next_ind(indent + 2, ' ');

		return std::visit(
			[&](auto const& arg) -> std::string {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return str_types::NULL_T;
				}
				else if constexpr (std::is_same_v<T, bool>) {
					return arg ? "true" : "false";
				}
				else if constexpr (std::is_arithmetic_v<T>) {
					return std::to_string(arg);
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					return "\"" + arg + "\"";
				}
				else if constexpr (std::is_same_v<T, Fraction>) {
					return std::to_string(arg.num) + "/" + std::to_string(arg.den);
				}
				else if constexpr (std::is_same_v<T, EnumValue>) {
					return "enum(" + std::to_string(arg.value)
						+ (arg.type_name.empty() ? "" : ("," + arg.type_name))
						+ ")";
				}
				else if constexpr (std::is_same_v<T, FlagsValue>) {
					return "flags(" + std::to_string(arg.value)
						+ (arg.type_name.empty() ? "" : ("," + arg.type_name))
						+ ")";
				}
				else if constexpr (std::is_same_v<T, CowBox<Array>>) {
					std::string res = "[\n";
					const auto& items = arg.get().items;
					for (size_t i = 0; i < items.size(); ++i) {
						res += next_ind + to_string(items[i], indent + 2);
						if (i < items.size() - 1) res += ",";
						res += "\n";
					}
					res += ind + "]";
					return res;
				}
				else if constexpr (std::is_same_v<T, CowBox<Map>>) {
					std::string res = "{\n";
					const auto& fields = arg.get().fields;
					size_t i = 0;
					for (const auto& [k, v] : fields) {
						res += next_ind + "\"" + k
							+ "\": " + to_string(v, indent + 2);
						if (++i < fields.size()) res += ",";
						res += "\n";
					}
					res += ind + "}";
					return res;
				}
				else {
					return "<unknown>";
				}
			},
			val.data
				);
	}

	// ============================================================
	// Интроспекция
	// ============================================================
	class ValueIntrospector
	{
	public:
		static std::vector<std::string> map_keys(const Value& val)
		{
			if (!val.is<CowBox<Map>>()) return {};
			std::vector<std::string> keys;
			for (const auto& [k, _] : val.as<CowBox<Map>>().get().fields) {
				keys.push_back(k);
			}
			return keys;
		}

		static size_t array_length(const Value& val)
		{
			if (!val.is<CowBox<Array>>()) return 0;
			return val.as<CowBox<Array>>().get().items.size();
		}

		static std::string type_name(const Value& val) { return val.typeid_name(); }
	};


	// ============================================================
	// Builder pattern
	// ============================================================
	class ValueBuilder
	{
		Value root_;
		std::vector<std::string> path_;

		// Вспомогательный метод для получения текущего контекста (куда писать)
		Value& current()
		{
			Value* current = &root_;
			for (const auto& key : path_) {
				if (!map_has_key(*current, key)) {
					map_set(*current, key, make_map());
				}
				current = &map_get(*current, key);
			}
			return *current;
		}

	public:
		ValueBuilder() :
			root_(make_map())
		{}

		explicit ValueBuilder(Value v) :
			root_(std::move(v))
		{}

		ValueBuilder& set(const std::string& key, Value v)
		{
			map_set(current(), key, std::move(v));
			return *this;
		}

		template<class U>
		ValueBuilder& set(const std::string& key, U&& u)
		{
			map_set(current(), key, wrap(std::forward<U>(u)));
			return *this;
		}

		ValueBuilder& push(Value v)
		{
			array_push(current(), std::move(v));
			return *this;
		}

		template<class U>
		ValueBuilder& push(U&& u)
		{
			array_push(current(), wrap(std::forward<U>(u)));
			return *this;
		}

		ValueBuilder& enter(const std::string& key)
		{
			path_.push_back(key);
			return *this;
		}

		ValueBuilder& exit()
		{
			if (!path_.empty()) path_.pop_back();
			return *this;
		}

		Value build() { return root_; }

		// Для отладки
		const Value& get_root() const { return root_; }
	};

}