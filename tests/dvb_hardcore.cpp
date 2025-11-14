#include <chrono>
#include <gtest/gtest.h>

#include <flexvalue/DVBDescriptorHelper.hpp>

namespace dvb_tests {

using namespace fxv;

// ============================================================
// Test Fixtures
// ============================================================

class DVBDescriptorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

// ============================================================
// 1. ISO 639 Language Descriptor (Tag: 0x0A) -真の修正
// ============================================================

TEST_F(DVBDescriptorTest, ISO639LanguageDescriptorSimple) {
  auto schema = std::make_shared<SchemaField>("language_descriptor", "map");
  schema->required = true;

  // Descriptor header
  auto tag = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag->required = true;
  tag->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x0A};
    return c;
  }());

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  // Language entries: array of language loops
  auto entries = std::make_shared<SchemaField>("language_entries", "array");
  auto entry = std::make_shared<SchemaField>("entry", "map");

  auto iso_lang_code =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang_code->required = true;
  iso_lang_code->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Length;
    c.min_val = 3;
    c.max_val = 3;
    return c;
  }());

  auto audio_type = std::make_shared<SchemaField>("audio_type", "int32");
  audio_type->required = true;
  audio_type->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x00, 0x01, 0x02, 0x03, 0x04};
    return c;
  }());

  entry->properties["ISO_639_language_code"] = iso_lang_code;
  entry->properties["audio_type"] = audio_type;
  entries->items = entry;

  schema->properties["descriptor_tag"] = tag;
  schema->properties["descriptor_length"] = length;
  schema->properties["language_entries"] = entries;

  // ✅ КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: Инициализируем language_entries как ARRAY перед
  // enter()
  ValueBuilder builder;
  builder.set("descriptor_tag", 0x0A);
  builder.set("descriptor_length", 8);

  // ✅ САМОЕ ВАЖНОЕ: Явно создаём Array!
  builder.set("language_entries", make_array());

  // Теперь enter() найдёт существующий Array, а не создаст Map
  builder.enter("language_entries");

  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "spa")
                   .set("audio_type", 0x00)
                   .build());

  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "fra")
                   .set("audio_type", 0x02)
                   .build());

  builder.exit();

  Value data = builder.build();

  // Validate
  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 2. AC-3 Audio Descriptor (Tag: 0x6A)
// ============================================================

TEST_F(DVBDescriptorTest, AC3AudioDescriptor) {
  auto schema = std::make_shared<SchemaField>("ac3_descriptor", "map");
  schema->required = true;

  auto tag = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto sample_rate_code =
      std::make_shared<SchemaField>("sample_rate_code", "int32");
  sample_rate_code->required = true;
  sample_rate_code->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0, 1, 2};
    return c;
  }());

  auto bsid = std::make_shared<SchemaField>("bsid", "int32");
  bsid->required = true;
  bsid->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Range;
    c.min_val = 0;
    c.max_val = 7;
    return c;
  }());

  auto bsmod = std::make_shared<SchemaField>("bsmod", "int32");
  bsmod->required = true;

  auto num_ind_sub = std::make_shared<SchemaField>("num_ind_sub", "int32");
  num_ind_sub->required = true;

  auto component_type =
      std::make_shared<SchemaField>("component_type", "int32");
  component_type->required = true;

  auto lfeon = std::make_shared<SchemaField>("lfeon", "bool");
  lfeon->required = true;

  schema->properties["descriptor_tag"] = tag;
  schema->properties["descriptor_length"] = length;
  schema->properties["sample_rate_code"] = sample_rate_code;
  schema->properties["bsid"] = bsid;
  schema->properties["bsmod"] = bsmod;
  schema->properties["num_ind_sub"] = num_ind_sub;
  schema->properties["component_type"] = component_type;
  schema->properties["lfeon"] = lfeon;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x6A);
  builder.set("descriptor_length", 3);
  builder.set("sample_rate_code", 0);
  builder.set("bsid", 0);
  builder.set("bsmod", 1);
  builder.set("num_ind_sub", 0);
  builder.set("component_type", 0x09);
  builder.set("lfeon", true);

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 3. Subtitling Descriptor (Tag: 0x59)
// ============================================================

TEST_F(DVBDescriptorTest, SubtitlingDescriptor) {
  auto schema = std::make_shared<SchemaField>("subtitling_descriptor", "map");
  schema->required = true;

  auto tag = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto entries = std::make_shared<SchemaField>("subtitle_entries", "array");
  auto entry = std::make_shared<SchemaField>("entry", "map");

  auto iso_lang =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang->required = true;

  auto sub_type = std::make_shared<SchemaField>("subtitling_type", "int32");
  sub_type->required = true;
  sub_type->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x10, 0x11, 0x20, 0x21};
    return c;
  }());

  auto composition_id =
      std::make_shared<SchemaField>("composition_page_id", "int32");
  composition_id->required = true;

  auto ancillary_id =
      std::make_shared<SchemaField>("ancillary_page_id", "int32");
  ancillary_id->required = true;

  entry->properties["ISO_639_language_code"] = iso_lang;
  entry->properties["subtitling_type"] = sub_type;
  entry->properties["composition_page_id"] = composition_id;
  entry->properties["ancillary_page_id"] = ancillary_id;
  entries->items = entry;

  schema->properties["descriptor_tag"] = tag;
  schema->properties["descriptor_length"] = length;
  schema->properties["subtitle_entries"] = entries;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x59);
  builder.set("descriptor_length", 16);

  // ✅ ИНИЦИАЛИЗИРУЕМ subtitle_entries как ARRAY
  builder.set("subtitle_entries", make_array());
  builder.enter("subtitle_entries");

  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "eng")
                   .set("subtitling_type", 0x10)
                   .set("composition_page_id", 0x0001)
                   .set("ancillary_page_id", 0x0002)
                   .build());
  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "fra")
                   .set("subtitling_type", 0x11)
                   .set("composition_page_id", 0x0003)
                   .set("ancillary_page_id", 0x0004)
                   .build());
  builder.exit();

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 4. Component Descriptor (Tag: 0x50)
// ============================================================

TEST_F(DVBDescriptorTest, ComponentDescriptor) {
  auto schema = std::make_shared<SchemaField>("component_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto stream_content =
      std::make_shared<SchemaField>("stream_content", "int32");
  stream_content->required = true;
  stream_content->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    return c;
  }());

  auto component_type =
      std::make_shared<SchemaField>("component_type", "int32");
  component_type->required = true;

  auto component_tag = std::make_shared<SchemaField>("component_tag", "int32");
  component_tag->required = true;

  auto iso_lang =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang->required = false;

  auto text = std::make_shared<SchemaField>("text_description", "string");
  text->required = false;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["stream_content"] = stream_content;
  schema->properties["component_type"] = component_type;
  schema->properties["component_tag"] = component_tag;
  schema->properties["ISO_639_language_code"] = iso_lang;
  schema->properties["text_description"] = text;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x50);
  builder.set("descriptor_length", 8);
  builder.set("stream_content", 0x05);
  builder.set("component_type", 0x0B);
  builder.set("component_tag", 0);
  builder.set("ISO_639_language_code", "und");
  builder.set("text_description", "Main Video");

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 5. Content Descriptor (Tag: 0x54)
// ============================================================

TEST_F(DVBDescriptorTest, ContentDescriptor) {
  auto schema = std::make_shared<SchemaField>("content_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto nibbles = std::make_shared<SchemaField>("content_nibbles", "array");
  auto nibble = std::make_shared<SchemaField>("nibble", "map");

  auto content_nibble_level_1 =
      std::make_shared<SchemaField>("content_nibble_level_1", "int32");
  content_nibble_level_1->required = true;
  content_nibble_level_1->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB};
    return c;
  }());

  auto content_nibble_level_2 =
      std::make_shared<SchemaField>("content_nibble_level_2", "int32");
  content_nibble_level_2->required = true;

  auto user_byte = std::make_shared<SchemaField>("user_byte", "int32");
  user_byte->required = false;

  nibble->properties["content_nibble_level_1"] = content_nibble_level_1;
  nibble->properties["content_nibble_level_2"] = content_nibble_level_2;
  nibble->properties["user_byte"] = user_byte;
  nibbles->items = nibble;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["content_nibbles"] = nibbles;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x54);
  builder.set("descriptor_length", 6);

  // ✅ ИНИЦИАЛИЗИРУЕМ content_nibbles как ARRAY
  builder.set("content_nibbles", make_array());
  builder.enter("content_nibbles");

  builder.push(ValueBuilder()
                   .set("content_nibble_level_1", 0x9)
                   .set("content_nibble_level_2", 0x7)
                   .set("user_byte", 0x00)
                   .build());
  builder.push(ValueBuilder()
                   .set("content_nibble_level_1", 0x4)
                   .set("content_nibble_level_2", 0x3)
                   .set("user_byte", 0x00)
                   .build());
  builder.exit();

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 6. Extended Event Descriptor (Tag: 0x4E)
// ============================================================

TEST_F(DVBDescriptorTest, ExtendedEventDescriptor) {
  auto schema =
      std::make_shared<SchemaField>("extended_event_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto descriptor_number =
      std::make_shared<SchemaField>("descriptor_number", "int32");
  descriptor_number->required = true;

  auto last_descriptor =
      std::make_shared<SchemaField>("last_descriptor_number", "int32");
  last_descriptor->required = true;

  auto iso_lang =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang->required = true;

  auto items = std::make_shared<SchemaField>("items", "array");
  auto item = std::make_shared<SchemaField>("item", "map");

  auto item_desc = std::make_shared<SchemaField>("item_description", "string");
  item_desc->required = true;

  auto item_text = std::make_shared<SchemaField>("item_text", "string");
  item_text->required = true;

  item->properties["item_description"] = item_desc;
  item->properties["item_text"] = item_text;
  items->items = item;

  auto text = std::make_shared<SchemaField>("event_text", "string");
  text->required = true;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["descriptor_number"] = descriptor_number;
  schema->properties["last_descriptor_number"] = last_descriptor;
  schema->properties["ISO_639_language_code"] = iso_lang;
  schema->properties["items"] = items;
  schema->properties["event_text"] = text;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x4E);
  builder.set("descriptor_length", 120);
  builder.set("descriptor_number", 0);
  builder.set("last_descriptor_number", 0);
  builder.set("ISO_639_language_code", "eng");

  // ✅ ИНИЦИАЛИЗИРУЕМ items как ARRAY
  builder.set("items", make_array());
  builder.enter("items");

  builder.push(ValueBuilder()
                   .set("item_description", "Director")
                   .set("item_text", "Martin Scorsese")
                   .build());
  builder.push(ValueBuilder()
                   .set("item_description", "Cast")
                   .set("item_text", "Robert De Niro, Al Pacino")
                   .build());
  builder.push(ValueBuilder()
                   .set("item_description", "Rating")
                   .set("item_text", "PG-13")
                   .build());
  builder.exit();

  builder.set("event_text", "A comprehensive documentary about the history of "
                            "organized crime in America");

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 7. Teletext Descriptor (Tag: 0x56)
// ============================================================

TEST_F(DVBDescriptorTest, TeletextDescriptor) {
  auto schema = std::make_shared<SchemaField>("teletext_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto entries = std::make_shared<SchemaField>("teletext_entries", "array");
  auto entry = std::make_shared<SchemaField>("entry", "map");

  auto iso_lang =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang->required = true;

  auto teletext_type = std::make_shared<SchemaField>("teletext_type", "int32");
  teletext_type->required = true;
  teletext_type->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0x00, 0x01, 0x02, 0x03, 0x04};
    return c;
  }());

  auto magazine_number =
      std::make_shared<SchemaField>("magazine_number", "int32");
  magazine_number->required = true;
  magazine_number->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Range;
    c.min_val = 1;
    c.max_val = 8;
    return c;
  }());

  auto page_number = std::make_shared<SchemaField>("page_number", "int32");
  page_number->required = true;

  entry->properties["ISO_639_language_code"] = iso_lang;
  entry->properties["teletext_type"] = teletext_type;
  entry->properties["magazine_number"] = magazine_number;
  entry->properties["page_number"] = page_number;
  entries->items = entry;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["teletext_entries"] = entries;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x56);
  builder.set("descriptor_length", 10);

  // ✅ ИНИЦИАЛИЗИРУЕМ teletext_entries как ARRAY
  builder.set("teletext_entries", make_array());
  builder.enter("teletext_entries");

  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "eng")
                   .set("teletext_type", 0x01)
                   .set("magazine_number", 8)
                   .set("page_number", 0x88)
                   .build());
  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", "spa")
                   .set("teletext_type", 0x04)
                   .set("magazine_number", 8)
                   .set("page_number", 0x89)
                   .build());
  builder.exit();

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 8. DTS Audio Descriptor (Tag: 0x7B)
// ============================================================

TEST_F(DVBDescriptorTest, DTSAudioDescriptor) {
  auto schema = std::make_shared<SchemaField>("dts_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto sample_rate_code =
      std::make_shared<SchemaField>("sample_rate_code", "int32");
  sample_rate_code->required = true;
  sample_rate_code->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Enum;
    c.allowed_values = {0, 1, 2, 3};
    return c;
  }());

  auto bit_rate_code = std::make_shared<SchemaField>("bit_rate_code", "int32");
  bit_rate_code->required = true;
  bit_rate_code->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Range;
    c.min_val = 0;
    c.max_val = 15;
    return c;
  }());

  auto surround_mode = std::make_shared<SchemaField>("surround_mode", "bool");
  surround_mode->required = true;

  auto lfe_channel = std::make_shared<SchemaField>("lfe_channel", "bool");
  lfe_channel->required = true;

  auto extended_surround =
      std::make_shared<SchemaField>("extended_surround", "bool");
  extended_surround->required = false;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["sample_rate_code"] = sample_rate_code;
  schema->properties["bit_rate_code"] = bit_rate_code;
  schema->properties["surround_mode"] = surround_mode;
  schema->properties["lfe_channel"] = lfe_channel;
  schema->properties["extended_surround"] = extended_surround;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x7B);
  builder.set("descriptor_length", 3);
  builder.set("sample_rate_code", 0);
  builder.set("bit_rate_code", 3);
  builder.set("surround_mode", true);
  builder.set("lfe_channel", true);
  builder.set("extended_surround", false);

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 9. Parental Rating Descriptor (Tag: 0x55)
// ============================================================

TEST_F(DVBDescriptorTest, ParentalRatingDescriptor) {
  auto schema =
      std::make_shared<SchemaField>("parental_rating_descriptor", "map");
  schema->required = true;

  auto tag_field = std::make_shared<SchemaField>("descriptor_tag", "int32");
  tag_field->required = true;

  auto length = std::make_shared<SchemaField>("descriptor_length", "int32");
  length->required = true;

  auto ratings = std::make_shared<SchemaField>("country_ratings", "array");
  auto rating = std::make_shared<SchemaField>("rating", "map");

  auto country_code = std::make_shared<SchemaField>("country_code", "string");
  country_code->required = true;
  country_code->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Length;
    c.min_val = 3;
    c.max_val = 3;
    return c;
  }());

  auto rating_value = std::make_shared<SchemaField>("rating", "int32");
  rating_value->required = true;

  rating->properties["country_code"] = country_code;
  rating->properties["rating"] = rating_value;
  ratings->items = rating;

  schema->properties["descriptor_tag"] = tag_field;
  schema->properties["descriptor_length"] = length;
  schema->properties["country_ratings"] = ratings;

  ValueBuilder builder;
  builder.set("descriptor_tag", 0x55);
  builder.set("descriptor_length", 20);

  // ✅ ИНИЦИАЛИЗИРУЕМ country_ratings как ARRAY
  builder.set("country_ratings", make_array());
  builder.enter("country_ratings");

  builder.push(
      ValueBuilder().set("country_code", "GBR").set("rating", 0x0D).build());
  builder.push(
      ValueBuilder().set("country_code", "FRA").set("rating", 0x0C).build());
  builder.push(
      ValueBuilder().set("country_code", "DEU").set("rating", 0x0C).build());
  builder.push(
      ValueBuilder().set("country_code", "ESP").set("rating", 0x0E).build());
  builder.exit();

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

// ============================================================
// 10. Complex PMT Entry
// ============================================================

TEST_F(DVBDescriptorTest, ComplexPMTEntry) {
  ValueBuilder pmt_builder;

  // Header
  pmt_builder.set("program_number", 1);
  pmt_builder.set("PMT_PID", 0x0100);
  pmt_builder.set("version_number", 1);

  // Program-level descriptors
  // ✅ ИНИЦИАЛИЗИРУЕМ как ARRAY
  pmt_builder.set("program_descriptors", make_array());
  pmt_builder.enter("program_descriptors");
  pmt_builder.push(ValueBuilder()
                       .set("descriptor_tag", 0x55)
                       .set("descriptor_length", 5)
                       .set("rating", 0x0D)
                       .build());
  pmt_builder.exit();

  // ES streams
  // ✅ ИНИЦИАЛИЗИРУЕМ как ARRAY
  pmt_builder.set("elementary_streams", make_array());
  pmt_builder.enter("elementary_streams");

  // Video stream (H.264/AVC)
  pmt_builder.push(ValueBuilder()
                       .set("stream_type", 0x1B)
                       .set("elementary_PID", 0x0101)
                       .set("es_descriptors", make_array())
                       .enter("es_descriptors")
                       .push(ValueBuilder()
                                 .set("descriptor_tag", 0x50)
                                 .set("stream_content", 0x05)
                                 .set("component_type", 0x0B)
                                 .build())
                       .exit()
                       .build());

  // Audio stream 1 (AC-3)
  pmt_builder.push(ValueBuilder()
                       .set("stream_type", 0x06)
                       .set("elementary_PID", 0x0102)
                       .set("es_descriptors", make_array())
                       .enter("es_descriptors")
                       .push(ValueBuilder()
                                 .set("descriptor_tag", 0x0A)
                                 .set("language", "eng")
                                 .set("audio_type", 0x00)
                                 .build())
                       .push(ValueBuilder()
                                 .set("descriptor_tag", 0x6A)
                                 .set("sample_rate", 0)
                                 .set("lfeon", true)
                                 .build())
                       .exit()
                       .build());

  // Audio stream 2 (hearing impaired)
  pmt_builder.push(ValueBuilder()
                       .set("stream_type", 0x06)
                       .set("elementary_PID", 0x0103)
                       .set("es_descriptors", make_array())
                       .enter("es_descriptors")
                       .push(ValueBuilder()
                                 .set("descriptor_tag", 0x0A)
                                 .set("language", "eng")
                                 .set("audio_type", 0x02)
                                 .build())
                       .exit()
                       .build());

  // Subtitle stream (DVB)
  pmt_builder.push(ValueBuilder()
                       .set("stream_type", 0x06)
                       .set("elementary_PID", 0x0104)
                       .set("es_descriptors", make_array())
                       .enter("es_descriptors")
                       .push(ValueBuilder()
                                 .set("descriptor_tag", 0x59)
                                 .set("language", "eng")
                                 .set("subtitling_type", 0x10)
                                 .build())
                       .exit()
                       .build());

  pmt_builder.exit();

  Value data = pmt_builder.build();

  // Verify traversal
  int count = 0;
  traverse(data, [&](const Value &val, const std::string &) {
    if (val.is<int32_t>()) {
      count++;
    }
  });

  EXPECT_GT(count, 0) << "Complex PMT structure should have integer values";
}

// ============================================================
// Parametrized tests for multiple language codes
// ============================================================

class LanguageCodeTest : public ::testing::TestWithParam<std::string> {};

TEST_P(LanguageCodeTest, ValidLanguageCode) {
  std::string lang_code = GetParam();

  auto schema = std::make_shared<SchemaField>("language_descriptor", "map");
  auto entries = std::make_shared<SchemaField>("language_entries", "array");
  auto entry = std::make_shared<SchemaField>("entry", "map");

  auto iso_lang =
      std::make_shared<SchemaField>("ISO_639_language_code", "string");
  iso_lang->required = true;
  iso_lang->constraints.push_back([]() {
    ValueConstraint c;
    c.type = ValueConstraint::Type::Length;
    c.min_val = 3;
    c.max_val = 3;
    return c;
  }());

  auto audio_type = std::make_shared<SchemaField>("audio_type", "int32");
  audio_type->required = true;

  entry->properties["ISO_639_language_code"] = iso_lang;
  entry->properties["audio_type"] = audio_type;
  entries->items = entry;

  schema->properties["language_entries"] = entries;

  ValueBuilder builder;

  // ✅ ИНИЦИАЛИЗИРУЕМ language_entries как ARRAY
  builder.set("language_entries", make_array());
  builder.enter("language_entries");
  builder.push(ValueBuilder()
                   .set("ISO_639_language_code", lang_code)
                   .set("audio_type", 0x00)
                   .build());
  builder.exit();

  Value data = builder.build();

  SchemaValidator validator(schema);
  auto errors = validator.validate(data);

  EXPECT_TRUE(errors.empty()) << validator.format_errors(errors);
}

INSTANTIATE_TEST_SUITE_P(DVBLanguages, LanguageCodeTest,
                         ::testing::Values("eng", "fra", "deu", "spa", "ita",
                                           "rus", "und", "qad"));

} // namespace dvb_tests
