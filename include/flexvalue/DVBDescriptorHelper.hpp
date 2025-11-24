#pragma once

/**
 * DVB Descriptor Helper Utilities
 *
 * Утилиты для работы с DVB дескрипторами
 * Содержит фабрики, валидаторы и конверторы для общих DVB типов
 */

#include <flexvalue/fxv_include.hpp>
#include <string>
#include <map>
#include <vector>

namespace dvb {

	using namespace fxv;

	// ============================================================
	// DVB Descriptor Tag Constants
	// ============================================================
	enum class DescriptorTag : uint8_t {
		NetworkName = 0x40,
		ServiceList = 0x41,
		Stuffing = 0x42,
		SatelliteDeliverySystem = 0x43,
		CableDeliverySystem = 0x44,
		VBIData = 0x45,
		VBITeletext = 0x46,
		BouquetName = 0x47,
		Service = 0x48,
		CountryAvailability = 0x49,
		Linkage = 0x4A,
		NVOD_Reference = 0x4B,
		TimeShiftedService = 0x4C,
		ShortEvent = 0x4D,
		ExtendedEvent = 0x4E,
		Component = 0x50,
		Mosaic = 0x51,
		StreamIdentifier = 0x52,
		CA_Identifier = 0x53,
		Content = 0x54,
		ParentalRating = 0x55,
		Teletext = 0x56,
		Telephone = 0x57,
		LocalTimeOffset = 0x58,
		Subtitling = 0x59,
		TerrestrialDeliverySystem = 0x5A,
		AC3 = 0x6A,
		DTS = 0x7B,
		ISO639Language = 0x0A
	};

	// ============================================================
	// ISO 639 Language Codes
	// ============================================================
	static const std::map<std::string, std::string> ISO639_CODES = {
		{"eng", "English"},
		{"fra", "French"},
		{"deu", "German"},
		{"ita", "Italian"},
		{"spa", "Spanish"},
		{"rus", "Russian"},
		{"pol", "Polish"},
		{"nld", "Dutch"},
		{"por", "Portuguese"},
		{"jpn", "Japanese"},
		{"tha", "Thai"},
		{"zho", "Chinese"},
		{"qaa", "Original audio"},
		{"qad", "Audio description"},
		{"qab", "Second audio"},
		{"qac", "Third audio"},
		{"und", "Undetermined"},
		{"mul", "Multiple languages"}
	};

	// ============================================================
	// ISO 3166 Country Codes
	// ============================================================
	static const std::map<std::string, std::string> ISO3166_CODES = {
		{"GBR", "United Kingdom"},
		{"FRA", "France"},
		{"DEU", "Germany"},
		{"ITA", "Italy"},
		{"ESP", "Spain"},
		{"NLD", "Netherlands"},
		{"RUS", "Russia"},
		{"POL", "Poland"},
		{"PRT", "Portugal"},
		{"JPN", "Japan"},
		{"THA", "Thailand"},
		{"CHN", "China"},
		{"USA", "United States"},
		{"CAN", "Canada"},
		{"AUS", "Australia"},
		{"IND", "India"}
	};

	// ============================================================
	// Helper: ISO 639 Language Descriptor Factory
	// ============================================================
	inline Value create_iso639_language_descriptor(
		const std::vector<std::pair<std::string, uint8_t>>& languages)
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::ISO639Language));
		builder.set("descriptor_length", static_cast<int32_t>(languages.size() * 4));

		builder.enter("language_entries");
		for (const auto& [lang_code, audio_type] : languages) {
			builder.push(
				ValueBuilder()
				.set("ISO_639_language_code", lang_code)
				.set("audio_type", audio_type)
				.build()
			);
		}
		builder.exit();

		return builder.build();
	}

	// ============================================================
	// Helper: Component Descriptor Factory
	// ============================================================
	inline Value create_component_descriptor(
		uint8_t stream_content,
		uint8_t component_type,
		uint8_t component_tag,
		const std::string& language = "und",
		const std::string& description = "")
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::Component));
		builder.set("descriptor_length", 6);
		builder.set("stream_content", stream_content);
		builder.set("component_type", component_type);
		builder.set("component_tag", component_tag);
		builder.set("ISO_639_language_code", language);
		if (!description.empty()) {
			builder.set("text_description", description);
		}

		return builder.build();
	}

	// ============================================================
	// Helper: Subtitling Descriptor Factory
	// ============================================================
	inline Value create_subtitling_descriptor(
		const std::vector<std::tuple<std::string, uint8_t, uint16_t, uint16_t>>& subtitles)
	{
		// subtitle tuple: (language, type, composition_id, ancillary_id)
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::Subtitling));
		builder.set("descriptor_length", static_cast<int32_t>(subtitles.size() * 8));

		builder.enter("subtitle_entries");
		for (const auto& [lang, type, comp_id, anc_id] : subtitles) {
			builder.push(
				ValueBuilder()
				.set("ISO_639_language_code", lang)
				.set("subtitling_type", type)
				.set("composition_page_id", static_cast<int32_t>(comp_id))
				.set("ancillary_page_id", static_cast<int32_t>(anc_id))
				.build()
			);
		}
		builder.exit();

		return builder.build();
	}

	// ============================================================
	// Helper: Content Descriptor Factory
	// ============================================================
	inline Value create_content_descriptor(
		const std::vector<std::pair<uint8_t, uint8_t>>& genres)
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::Content));
		builder.set("descriptor_length", static_cast<int32_t>(genres.size() * 3));

		builder.enter("content_nibbles");
		for (const auto& [level1, level2] : genres) {
			builder.push(
				ValueBuilder()
				.set("content_nibble_level_1", level1)
				.set("content_nibble_level_2", level2)
				.set("user_byte", 0)
				.build()
			);
		}
		builder.exit();

		return builder.build();
	}

	// ============================================================
	// Helper: Parental Rating Descriptor Factory
	// ============================================================
	inline Value create_parental_rating_descriptor(
		const std::map<std::string, uint8_t>& country_ratings)
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::ParentalRating));
		builder.set("descriptor_length", static_cast<int32_t>(country_ratings.size() * 5));

		builder.enter("country_ratings");
		for (const auto& [country, rating] : country_ratings) {
			builder.push(
				ValueBuilder()
				.set("country_code", country)
				.set("rating", rating)
				.build()
			);
		}
		builder.exit();

		return builder.build();
	}

	// ============================================================
	// Helper: AC-3 Descriptor Factory
	// ============================================================
	inline Value create_ac3_descriptor(
		uint8_t sample_rate_code,
		uint8_t bsid,
		uint8_t bsmod,
		uint8_t component_type,
		bool lfeon = true)
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::AC3));
		builder.set("descriptor_length", 3);
		builder.set("sample_rate_code", sample_rate_code);
		builder.set("bsid", bsid);
		builder.set("bsmod", bsmod);
		builder.set("num_ind_sub", 0);
		builder.set("component_type", component_type);
		builder.set("lfeon", lfeon);

		return builder.build();
	}

	// ============================================================
	// Helper: DTS Descriptor Factory
	// ============================================================
	inline Value create_dts_descriptor(
		uint8_t sample_rate_code,
		uint8_t bit_rate_code,
		bool surround_mode = true,
		bool lfe_channel = true,
		bool extended_surround = false)
	{
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::DTS));
		builder.set("descriptor_length", 3);
		builder.set("sample_rate_code", sample_rate_code);
		builder.set("bit_rate_code", bit_rate_code);
		builder.set("surround_mode", surround_mode);
		builder.set("lfe_channel", lfe_channel);
		builder.set("extended_surround", extended_surround);

		return builder.build();
	}

	// ============================================================
	// Helper: Teletext Descriptor Factory
	// ============================================================
	inline Value create_teletext_descriptor(
		const std::vector<std::tuple<std::string, uint8_t, uint8_t, uint8_t>>& teletext_pages)
	{
		// tuple: (language, type, magazine, page)
		ValueBuilder builder;
		builder.set("descriptor_tag", static_cast<int32_t>(DescriptorTag::Teletext));
		builder.set("descriptor_length", static_cast<int32_t>(teletext_pages.size() * 5));

		builder.enter("teletext_entries");
		for (const auto& [lang, type, mag, page] : teletext_pages) {
			builder.push(
				ValueBuilder()
				.set("ISO_639_language_code", lang)
				.set("teletext_type", type)
				.set("magazine_number", mag)
				.set("page_number", page)
				.build()
			);
		}
		builder.exit();

		return builder.build();
	}

	// ============================================================
	// Validation Helpers
	// ============================================================

	/// Проверяет, является ли код языка валидным ISO 639
	inline bool is_valid_language_code(const std::string& code)
	{
		if (code.length() != 3) return false;
		return ISO639_CODES.count(code) > 0 || code == "und";
	}

	/// Проверяет, является ли код страны валидным ISO 3166
	inline bool is_valid_country_code(const std::string& code)
	{
		if (code.length() != 3) return false;
		return ISO3166_CODES.count(code) > 0;
	}

	/// Получить человеческое описание языка
	inline std::string get_language_name(const std::string& code)
	{
		auto it = ISO639_CODES.find(code);
		return (it != ISO639_CODES.end()) ? it->second : "Unknown";
	}

	/// Получить человеческое описание страны
	inline std::string get_country_name(const std::string& code)
	{
		auto it = ISO3166_CODES.find(code);
		return (it != ISO3166_CODES.end()) ? it->second : "Unknown";
	}

	// ============================================================
	// Conversion Helpers
	// ============================================================

	/// Конвертировать возраст в DVB рейтинг (0x04-0x12 = 7-18 лет)
	inline uint8_t age_to_dvb_rating(int age)
	{
		if (age < 4) return 0x00;      // undefined
		if (age > 18) return 0x12;     // max standard value
		return static_cast<uint8_t>(age + 3);
	}

	/// Конвертировать DVB рейтинг обратно в возраст
	inline int dvb_rating_to_age(uint8_t rating)
	{
		if (rating < 0x04 || rating > 0x12) return -1;
		return rating - 3;
	}

	/// Преобразовать выборку AC-3 в Hz
	inline uint32_t ac3_sample_rate(uint8_t code)
	{
		switch (code & 0x3) {
		case 0: return 48000;
		case 1: return 44100;
		case 2: return 32000;
		default: return 0;
		}
	}

	/// Преобразовать выборку DTS в Hz
	inline uint32_t dts_sample_rate(uint8_t code)
	{
		switch (code & 0x7) {
		case 0: return 48000;
		case 1: return 44100;
		case 2: return 32000;
		case 3: return 96000;
		default: return 0;
		}
	}

	// ============================================================
	// Schema Builders (for reuse)
	// ============================================================

	/// Создать схему для ISO 639 Language Descriptor
	inline std::shared_ptr<SchemaField> create_iso639_schema()
	{
		using namespace fxv;

		auto schema = std::make_shared<SchemaField>("language_descriptor", str_types::MAP_T);
		schema->required = true;

		auto tag = std::make_shared<SchemaField>("descriptor_tag", str_types::INT32_T);
		tag->required = true;

		auto length = std::make_shared<SchemaField>("descriptor_length", str_types::INT32_T);
		length->required = true;

		auto entries = std::make_shared<SchemaField>("language_entries", str_types::ARRAY_T);
		auto entry = std::make_shared<SchemaField>("entry", str_types::MAP_T);

		auto iso_lang = std::make_shared<SchemaField>("ISO_639_language_code", str_types::STRING_T);
		iso_lang->required = true;

		auto audio_type = std::make_shared<SchemaField>("audio_type", str_types::INT32_T);
		audio_type->required = true;

		entry->properties["ISO_639_language_code"] = iso_lang;
		entry->properties["audio_type"] = audio_type;
		entries->items = entry;

		schema->properties["descriptor_tag"] = tag;
		schema->properties["descriptor_length"] = length;
		schema->properties["language_entries"] = entries;

		return schema;
	}

	/// Вывести дескриптор в читаемом формате
	inline std::string format_descriptor(const Value& descriptor)
	{
		std::string result;

		if (!descriptor.is<CowBox<Map>>()) {
			return "Invalid descriptor";
		}

		auto& fields = descriptor.as<CowBox<Map>>().get().fields;

		if (fields.count("descriptor_tag")) {
			int tag = fields.at("descriptor_tag").as<int32_t>();
			result += "Descriptor [0x" + std::to_string(tag) + "]\n";
		}

		if (fields.count("descriptor_length")) {
			int len = fields.at("descriptor_length").as<int32_t>();
			result += "  Length: " + std::to_string(len) + " bytes\n";
		}

		// Print other fields
		for (const auto& [key, value] : fields) {
			if (key == "descriptor_tag" || key == "descriptor_length") continue;

			result += "  " + key + ": ";
			if (value.is<std::string>()) {
				result += value.as<std::string>();
			}
			else if (value.is<int32_t>()) {
				result += std::to_string(value.as<int32_t>());
			}
			else if (value.is<bool>()) {
				result += value.as<bool>() ? "true" : "false";
			}
			result += "\n";
		}

		return result;
	}

} // namespace dvb
