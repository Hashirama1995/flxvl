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
	// Schema validation (простой вариант)
	// ============================================================
	struct Schema
	{
		std::string type; // "map", "array", "int32", "string", etc.
		std::map<std::string, std::unique_ptr<Schema>> properties; // для map
		std::unique_ptr<Schema> items;                             // для array
		bool required = false;

		bool validate(
			const Value& val, std::string& error, const std::string& path = "/"
		) const
		{
			if (val.is_null() && !required) return true;
			if (val.is_null() && required) {
				error = "Required field is null at " + path;
				return false;
			}

			if (type == "map") {
				if (!val.is<CowBox<Map>>()) {
					error
						= "Expected map, got " + val.typeid_name() + " at " + path;
					return false;
				}
				const auto& m = val.as<CowBox<Map>>().get().fields;
				for (const auto& [k, v] : m) {
					if (properties.count(k)) {
						if (!properties.at(k)->validate(v, error, path + "/" + k))
							return false;
					}
				}
				return true;
			}
			else if (type == "array") {
				if (!val.is<CowBox<Array>>()) {
					error = "Expected array, got " + val.typeid_name() + " at "
						+ path;
					return false;
				}
				if (items) {
					const auto& arr = val.as<CowBox<Array>>().get().items;
					for (size_t i = 0; i < arr.size(); ++i) {
						if (!items->validate(
							arr[i], error, path + "[" + std::to_string(i) + "]"
						))
							return false;
					}
				}
				return true;
			}
			else if (type == "int32") {
				if (!val.is<int32_t>()) {
					error = "Expected int32, got " + val.typeid_name() + " at "
						+ path;
					return false;
				}
			}
			else if (type == "string") {
				if (!val.is<std::string>()) {
					error = "Expected string, got " + val.typeid_name() + " at "
						+ path;
					return false;
				}
			}
			return true;
		}
	};
	// ============================================================
	// Расширенная валидация с поддержкой DVB дескрипторов
	// ============================================================

	enum class ValidationSeverity { Error, Warning, Info };

	struct ValidationError {
		std::string path;
		std::string field_name;
		std::string expected;
		std::string actual;
		std::string reason;
		std::string suggestion;
		ValidationSeverity severity = ValidationSeverity::Error;

		std::string to_string() const
		{
			std::string result = "[" + path + "] ";
			if (!field_name.empty()) result += field_name + ": ";
			result += reason;
			if (!suggestion.empty()) result += " (hint: " + suggestion + ")";
			return result;
		}
	};

	struct ValueConstraint {
		enum class Type { None, Range, Length, Enum, Pattern, Custom };

		Type type = Type::None;
		int64_t min_val = LLONG_MIN;
		int64_t max_val = LLONG_MAX;
		std::vector<int64_t> allowed_values;
		std::string regex_pattern;
		std::function<bool(const Value&)> custom_fn;

		bool check(const Value& v) const
		{
			if (type == Type::None) return true;

			if (type == Type::Range) {
				if (v.is<int32_t>()) {
					int32_t val = v.as<int32_t>();
					return val >= min_val && val <= max_val;
				}
				if (v.is<int64_t>()) {
					int64_t val = v.as<int64_t>();
					return val >= min_val && val <= max_val;
				}
				return false;
			}

			if (type == Type::Length) {
				if (v.is<std::string>()) {
					size_t len = v.as<std::string>().length();
					return len >= static_cast<size_t>(min_val)
						&& len <= static_cast<size_t>(max_val);
				}
				if (v.is<CowBox<Array>>()) {
					size_t len = v.as<CowBox<Array>>().get().items.size();
					return len >= static_cast<size_t>(min_val)
						&& len <= static_cast<size_t>(max_val);
				}
				return false;
			}

			if (type == Type::Enum) {
				if (v.is<int32_t>()) {
					int32_t val = v.as<int32_t>();
					return std::find(allowed_values.begin(), allowed_values.end(),
						static_cast<int64_t>(val))
						!= allowed_values.end();
				}
				return false;
			}

			if (type == Type::Custom && custom_fn) {
				return custom_fn(v);
			}

			return true;
		}

		std::string describe() const
		{
			switch (type) {
			case Type::Range:
				return "range [" + std::to_string(min_val) + ".."
					+ std::to_string(max_val) + "]";
			case Type::Length:
				return "length [" + std::to_string(min_val) + ".."
					+ std::to_string(max_val) + "]";
			case Type::Enum:
				return "enum values";
			case Type::Pattern:
				return "pattern /" + regex_pattern + "/";
			case Type::Custom:
				return "custom constraint";
			default:
				return "no constraint";
			}
		}
	};

	struct SchemaField {
		std::string name;
		std::string type_name; // "int32", "string", "map", "array", etc
		bool required = false;

		// Условие наличия поля (зависит от других полей в parent)
		std::function<bool(const Value& parent_map)> condition;

		// Ограничения значения
		std::vector<ValueConstraint> constraints;

		// Для map: вложенные поля
		std::map<std::string, std::shared_ptr<SchemaField>> properties;

		// Для array: схема элементов
		std::shared_ptr<SchemaField> items;

		// Метаинформация
		std::string description;
		std::string units;
		std::string default_value_desc;
		bool is_computed = false;  // вычисляется ли автоматически?

		SchemaField(const std::string& n = "", const std::string& t = "")
			: name(n), type_name(t)
		{
		}

		bool validate_type(const Value& v, std::string& error) const
		{
			if (type_name == "null") return v.is_null();
			if (type_name == "bool") return v.is<bool>();
			if (type_name == "int32") return v.is<int32_t>();
			if (type_name == "uint32") return v.is<uint32_t>();
			if (type_name == "int64") return v.is<int64_t>();
			if (type_name == "uint64") return v.is<uint64_t>();
			if (type_name == "float") return v.is<float>();
			if (type_name == "double") return v.is<double>();
			if (type_name == "string") return v.is<std::string>();
			if (type_name == "fraction") return v.is<Fraction>();
			if (type_name == "enum") return v.is<EnumValue>();
			if (type_name == "flags") return v.is<FlagsValue>();
			if (type_name == "array") return v.is<CowBox<Array>>();
			if (type_name == "map") return v.is<CowBox<Map>>();

			error = "Unknown type: " + type_name;
			return false;
		}
	};

	class SchemaValidator {
	public:
		using ErrorList = std::vector<ValidationError>;
		SchemaValidator(std::shared_ptr<SchemaField> root_schema)
			: root_(root_schema)
		{
		}

		// Валидация: вернуть ВСЕ ошибки (batch validation)
		ErrorList validate(const Value& data, const std::string& path = "/") const
		{
			ErrorList errors;
			validate_recursive(data, root_, path, errors);
			return errors;
		}

		// Быстрая проверка: вернуть true/false на первой ошибке
		bool validate_quick(const Value& data, std::string& error,
			const std::string& path = "/") const
		{
			ErrorList errors;
			if (!validate_recursive_early_exit(data, root_, path, error)) {
				return false;
			}
			return true;
		}

		// Красивый вывод всех ошибок
		std::string format_errors(const ErrorList& errors) const
		{
			if (errors.empty()) return "Validation passed.";

			std::string result = "Validation failed with " + std::to_string(errors.size())
				+ " error(s):\n";
			for (size_t i = 0; i < errors.size(); ++i) {
				result += "  [" + std::to_string(i + 1) + "] "
					+ errors[i].to_string() + "\n";
			}
			return result;
		}

	private:
		std::shared_ptr<SchemaField> root_;

		void validate_recursive(const Value& val,
			std::shared_ptr<SchemaField> schema,
			const std::string& path, ErrorList& errors) const
		{
			if (!schema) return;

			// 1. Проверка типа
			std::string type_error;
			if (!schema->validate_type(val, type_error)) {
				ValidationError err;
				err.path = path;
				err.field_name = schema->name;
				err.expected = schema->type_name;
				err.actual = val.typeid_name();
				err.reason = "Type mismatch: expected " + schema->type_name + ", got "
					+ val.typeid_name();
				errors.push_back(err);
				return;  // Дальше не проверяем, если тип не совпадает
			}

			// 2. Проверка ограничений
			for (const auto& constraint : schema->constraints) {
				if (!constraint.check(val)) {
					ValidationError err;
					err.path = path;
					err.field_name = schema->name;
					err.reason = "Constraint violation: " + constraint.describe();
					errors.push_back(err);
				}
			}

			// 3. Рекурсивная проверка map
			if (schema->type_name == "map" && val.is<CowBox<Map>>()) {
				const auto& fields = val.as<CowBox<Map>>().get().fields;

				// Проверяем определённые в схеме поля
				for (const auto& [key, field_schema] : schema->properties) {
					bool should_exist = !field_schema->condition
						|| field_schema->condition(val);

					if (fields.count(key)) {
						validate_recursive(fields.at(key), field_schema,
							path + "/" + key, errors);
					}
					else if (field_schema->required && should_exist) {
						ValidationError err;
						err.path = path;
						err.field_name = key;
						err.reason
							= "Required field missing: " + key;
						if (!field_schema->description.empty())
							err.suggestion = field_schema->description;
						errors.push_back(err);
					}
				}
			}

			// 4. Рекурсивная проверка array
			if (schema->type_name == "array" && val.is<CowBox<Array>>()
				&& schema->items) {
				const auto& items = val.as<CowBox<Array>>().get().items;
				for (size_t i = 0; i < items.size(); ++i) {
					validate_recursive(items[i], schema->items,
						path + "[" + std::to_string(i) + "]",
						errors);
				}
			}
		}

		bool validate_recursive_early_exit(const Value& val,
			std::shared_ptr<SchemaField> schema,
			const std::string& path,
			std::string& error) const
		{
			if (!schema) return true;

			// Тип
			if (!schema->validate_type(val, error)) {
				error = "Type mismatch at " + path + ": expected "
					+ schema->type_name + ", got " + val.typeid_name();
				return false;
			}

			// Ограничения
			for (const auto& constraint : schema->constraints) {
				if (!constraint.check(val)) {
					error = "Constraint violation at " + path + ": "
						+ constraint.describe();
					return false;
				}
			}

			// map
			if (schema->type_name == "map" && val.is<CowBox<Map>>()) {
				const auto& fields = val.as<CowBox<Map>>().get().fields;
				for (const auto& [key, field_schema] : schema->properties) {
					if (fields.count(key)) {
						if (!validate_recursive_early_exit(fields.at(key),
							field_schema,
							path + "/" + key, error))
							return false;
					}
					else if (field_schema->required) {
						error = "Required field missing at " + path + "/" + key;
						return false;
					}
				}
			}

			// array
			if (schema->type_name == "array" && val.is<CowBox<Array>>()
				&& schema->items) {
				const auto& items = val.as<CowBox<Array>>().get().items;
				for (size_t i = 0; i < items.size(); ++i) {
					if (!validate_recursive_early_exit(
						items[i], schema->items,
						path + "[" + std::to_string(i) + "]", error))
						return false;
				}
			}

			return true;
		}

	};

	// ============================================================
	// Helper для построения схем (builder-like API)
	// ============================================================
	class SchemaBuilder {
		std::shared_ptr<SchemaField> schema_;

	public:
		SchemaBuilder(const std::string& name, const std::string& type)
			: schema_(std::make_shared<SchemaField>(name, type))
		{
		}

		static SchemaBuilder map_schema(const std::string& name = "root")
		{
			return SchemaBuilder(name, "map");
		}

		static SchemaBuilder array_schema(const std::string& name = "")
		{
			return SchemaBuilder(name, "array");
		}

		SchemaBuilder& required(bool r = true)
		{
			schema_->required = r;
			return *this;
		}

		SchemaBuilder& description(const std::string& desc)
		{
			schema_->description = desc;
			return *this;
		}

		SchemaBuilder& add_field(const std::string& key,
			std::shared_ptr<SchemaField> field_schema)
		{
			if (schema_->type_name != "map") {
				throw std::runtime_error("add_field: schema is not a map");
			}
			schema_->properties[key] = field_schema;
			return *this;
		}

		SchemaBuilder& set_items(std::shared_ptr<SchemaField> item_schema)
		{
			if (schema_->type_name != "array") {
				throw std::runtime_error("set_items: schema is not an array");
			}
			schema_->items = item_schema;
			return *this;
		}

		SchemaBuilder& add_constraint(const ValueConstraint& constraint)
		{
			schema_->constraints.push_back(constraint);
			return *this;
		}

		SchemaBuilder& add_range_constraint(int64_t min_v, int64_t max_v)
		{
			ValueConstraint c;
			c.type = ValueConstraint::Type::Range;
			c.min_val = min_v;
			c.max_val = max_v;
			schema_->constraints.push_back(c);
			return *this;
		}

		SchemaBuilder& add_length_constraint(size_t min_len, size_t max_len)
		{
			ValueConstraint c;
			c.type = ValueConstraint::Type::Length;
			c.min_val = static_cast<int64_t>(min_len);
			c.max_val = static_cast<int64_t>(max_len);
			schema_->constraints.push_back(c);
			return *this;
		}

		SchemaBuilder& add_enum_constraint(
			const std::vector<int64_t>& allowed)
		{
			ValueConstraint c;
			c.type = ValueConstraint::Type::Enum;
			c.allowed_values = allowed;
			schema_->constraints.push_back(c);
			return *this;
		}

		SchemaBuilder& condition(
			std::function<bool(const Value&)> cond)
		{
			schema_->condition = cond;
			return *this;
		}

		std::shared_ptr<SchemaField> build() { return schema_; }

	};

}