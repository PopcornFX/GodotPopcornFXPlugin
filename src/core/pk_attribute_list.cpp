//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_attribute_list.h"

#include "core/samplers/pk_attribute_sampler_audio.h"
#include "core/samplers/pk_attribute_sampler_image.h"
#include "core/samplers/pk_attribute_sampler_shape.h"
#include "integration/pk_error_handling.h"
#include "scene/pk_emitter_3d.h"

#include <pk_particles/include/ps_attributes.h>

namespace godot {

PKAttributeDesc::PKAttributeDesc() :
		// Explicit initializer list because Godot skips default values when (de)serialising, leading to garbage values.
		name(""), index(0), type(0), semantic(0), uid(CGuid::INVALID), default_value(PackedByteArray()) {}

PKAttributeDesc *PKAttributeDesc::from_declaration(const CParticleAttributeDeclaration *p_declaration, int p_index) {
	PKAttributeDesc *desc = memnew(PKAttributeDesc);
	desc->set_name(p_declaration->ExportedName().Data());
	desc->set_type(p_declaration->ExportedType());
	desc->set_semantic(p_declaration->GetEffectiveDataSemantic());
	desc->set_uid(p_declaration->UID());
	desc->set_index(p_index);

	const SAttributesContainer_SAttrib default_value = p_declaration->GetDefaultValue();
	const int byte_count = sizeof(default_value);
	desc->default_value.resize(byte_count);
	memcpy(desc->default_value.ptrw(), &default_value, byte_count);

	return desc;
}

SAttributesContainer_SAttrib PKAttributeDesc::default_value_as_attrib() const {
	SAttributesContainer_SAttrib attrib;
	memcpy(&attrib, default_value.ptr(), sizeof(attrib));
	return attrib;
}

void PKAttributeDesc::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeDesc, INT, type, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeDesc, INT, semantic, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeDesc, INT, index, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeDesc, INT, uid, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeDesc, STRING, name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeDesc, PACKED_BYTE_ARRAY, default_value, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
}

PKAttributeSamplerDesc::PKAttributeSamplerDesc() :
		// Explicit initializer list because Godot skips default values when (de)serialising, leading to garbage values.
		type(0), index(0), uid(CGuid::INVALID), name("") {}

PKAttributeSamplerDesc *PKAttributeSamplerDesc::from_declaration(const CParticleAttributeSamplerDeclaration *p_declaration, int p_index) {
	PKAttributeSamplerDesc *desc = memnew(PKAttributeSamplerDesc);
	desc->set_name(p_declaration->ExportedName().Data());
	desc->set_type(p_declaration->ExportedType());
	desc->set_uid(p_declaration->UID());
	desc->set_index(p_index);

	return desc;
}

void PKAttributeSamplerDesc::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeSamplerDesc, INT, type, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerDesc, INT, index, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerDesc, INT, uid, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerDesc, STRING, name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
}

void PKAttributeList::set_emitter(PKEmitter3D *p_emitter) {
	emitter = p_emitter;

	for (int i = 0; i < attribute_sampler_descs.size(); i++) {
		const Ref<PKAttributeSampler> sampler = get_attribute_sampler(i);
		if (sampler.is_null()) {
			continue;
		}
		sampler->set_emitter(emitter);
	}
}

void PKAttributeList::set_attribute_samplers(TypedArray<PKAttributeSampler> p_attribute_samplers) {
	attribute_samplers = p_attribute_samplers;
	for (int i = 0; i < p_attribute_samplers.size(); i++) {
		Ref<PKAttributeSampler> sampler = p_attribute_samplers[i];
		if (!sampler.is_valid()) {
			if (!PKGD_VERIFY(i < effect->get_effect()->GetAttributeSamplerCount())) {
				continue;
			}
			sampler = create_default_sampler(effect->get_effect()->GetAttributeSamplerDecl(i));
		}
		set_attribute_sampler_raw(i, sampler);
	}
}

SAttributesContainer_SAttrib *PKAttributeList::attribute_raw_data() {
	return reinterpret_cast<SAttributesContainer_SAttrib *>(attribute_data.ptrw());
}

const SAttributesContainer_SAttrib *PKAttributeList::attribute_raw_data() const {
	return reinterpret_cast<const SAttributesContainer_SAttrib *>(attribute_data.ptr());
}

PKAttributeList::PKAttributeList() {
	emitter = nullptr;
}

Ref<PKAttributeList> PKAttributeList::default_for_emitter(PKEmitter3D *p_emitter) {
	const Ref<PKEffect> effect = p_emitter->effect;
	if (!PKGD_VERIFY(effect->get_effect() != nullptr)) {
		return nullptr;
	}
	Ref<PKAttributeList> default_list;
	default_list.instantiate();
	default_list->effect = effect;
	default_list->emitter = p_emitter;

	// Attributes
	for (int i = 0; i < effect->get_effect()->GetAttributeCount(); i++) {
		default_list->attribute_descs.append(PKAttributeDesc::from_declaration(effect->get_effect()->GetAttributeDecl(i), i));
	}

	const SAttributesContainer *default_attributes = *(effect->get_effect()->AttributeFlatList()->DefaultAttributes());
	ERR_FAIL_COND_V(default_attributes == nullptr, nullptr);

	const int byte_count = default_attributes->Attributes().CoveredBytes();
	default_list->attribute_data.resize(byte_count);
	memcpy(default_list->attribute_data.ptrw(), default_attributes->Attributes().Data(), byte_count);

	// Attribute Samplers
	default_list->attribute_sampler_descs.resize(effect->get_effect()->GetAttributeSamplerCount());
	default_list->attribute_samplers.resize(default_list->attribute_sampler_descs.size());
	for (int i = 0; i < default_list->attribute_sampler_descs.size(); i++) {
		const CParticleAttributeSamplerDeclaration *decl = effect->get_effect()->GetAttributeSamplerDecl(i);
		default_list->attribute_sampler_descs[i] = PKAttributeSamplerDesc::from_declaration(decl, i);
		default_list->attribute_samplers[i] = create_default_sampler(decl);
	}

	return default_list;
}

Ref<PKAttributeDesc> PKAttributeList::get_attribute_desc(uint32_t p_id) {
	if (p_id >= attribute_descs.size()) {
		return nullptr;
	}
	return attribute_descs[p_id];
}

Ref<PKAttributeSamplerDesc> PKAttributeList::get_attribute_sampler_desc(uint32_t p_id) {
	if (p_id >= attribute_sampler_descs.size()) {
		return nullptr;
	}
	return attribute_sampler_descs[p_id];
}

Ref<PKAttributeDesc> PKAttributeList::get_attribute_desc(const String &p_name) {
	for (int i = 0; i < attribute_descs.size(); i++) {
		const Ref<PKAttributeDesc> desc = get_attribute_desc(i);
		if (desc->get_name() == p_name) {
			return desc;
		}
	}
	return nullptr;
}

Ref<PKAttributeSamplerDesc> PKAttributeList::get_attribute_sampler_desc(const String &p_name) {
	for (int i = 0; i < attribute_sampler_descs.size(); i++) {
		const Ref<PKAttributeSamplerDesc> desc = get_attribute_sampler_desc(i);
		if (desc->get_name() == p_name) {
			return desc;
		}
	}
	return nullptr;
}

Ref<PKAttributeDesc> PKAttributeList::get_attribute_desc_by_uid(uint32_t p_uid) {
	for (int i = 0; i < attribute_descs.size(); i++) {
		const Ref<PKAttributeDesc> desc = get_attribute_desc(i);
		if (desc->get_uid() == p_uid) {
			return desc;
		}
	}
	return nullptr;
}

Ref<PKAttributeSamplerDesc> PKAttributeList::get_attribute_sampler_desc_by_uid(uint32_t p_uid) {
	for (int i = 0; i < attribute_sampler_descs.size(); i++) {
		const Ref<PKAttributeSamplerDesc> desc = get_attribute_sampler_desc(i);
		if (desc->get_uid() == p_uid) {
			return desc;
		}
	}
	return nullptr;
}

CParticleAttributeList::_TypeOfAttributeAndSamplerList PKAttributeList::all_declarations() const {
	if (effect.is_null()) {
		return {};
	}
	return effect->get_effect()->AttributeFlatList()->AttributeAndSamplerList();
}

TMemoryView<CParticleAttributeDeclaration *const> PKAttributeList::all_attribute_declarations() const {
	if (effect.is_null()) {
		return {};
	}
	return effect->get_effect()->AttributeFlatList()->UniqueAttributeList();
}

TMemoryView<CParticleAttributeSamplerDeclaration *const> PKAttributeList::all_attribute_sampler_declarations() const {
	if (effect.is_null()) {
		return {};
	}
	return effect->get_effect()->AttributeFlatList()->UniqueSamplerList();
}

void PKAttributeList::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeList, OBJECT, effect, PROPERTY_HINT_RESOURCE_TYPE, "PKEffect", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeList, ARRAY, attribute_descs, PROPERTY_HINT_ARRAY_TYPE, "PKAttributeDesc", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeList, PACKED_BYTE_ARRAY, attribute_data, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeList, ARRAY, attribute_sampler_descs, PROPERTY_HINT_ARRAY_TYPE, "PKAttributeSamplerDesc", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeList, ARRAY, attribute_samplers, PROPERTY_HINT_ARRAY_TYPE, "PKAttributeSampler", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	ClassDB::bind_method(D_METHOD("get_attribute", "name"), &PKAttributeList::get_attribute_variant);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &PKAttributeList::set_attribute_variant);
}

void PKAttributeList::_physics_process() {
	for (const Ref<PKAttributeSampler> &sampler : attribute_samplers) {
		if (sampler.is_valid()) {
			sampler->_physics_process();
		}
	}
}

void PKAttributeList::_ready() {
	if (attribute_samplers.is_empty()) {
		return;
	}
	for (const Ref<PKAttributeSampler> &sampler : attribute_samplers) {
		if (sampler.is_valid()) {
			sampler->_ready();
		}
	}
}

bool attribs_match(const SAttributesContainer_SAttrib &p_a, const SAttributesContainer_SAttrib &p_b, int p_type, int p_dimension) {
	// For bool types we check int value instead of raw/boolean because popcorn doesn't use a standard value for bools
	// We also use != 0 instead of directly comparing because msvc doesn't respect C++ specs and uses a raw value compare instead of checking for non-zero
	switch (p_type) {
		case PopcornFX::BaseType_Bool:
		case PopcornFX::BaseType_Bool2:
		case PopcornFX::BaseType_Bool3:
		case PopcornFX::BaseType_Bool4:
			switch (p_dimension) {
				case 1:
					return (p_a.m_Data8b[0] != 0) == (p_b.m_Data8b[0] != 0);
				case 2:
					return (p_a.m_Data8b[0] != 0) == (p_b.m_Data8b[0] != 0) &&
							(p_a.m_Data8b[1] != 0) == (p_b.m_Data8b[1] != 0);
				case 3:
					return (p_a.m_Data8b[0] != 0) == (p_b.m_Data8b[0] != 0) &&
							(p_a.m_Data8b[1] != 0) == (p_b.m_Data8b[1] != 0) &&
							(p_a.m_Data8b[2] != 0) == (p_b.m_Data8b[2] != 0);
				case 4:
					return (p_a.m_Data8b[0] != 0) == (p_b.m_Data8b[0] != 0) &&
							(p_a.m_Data8b[1] != 0) == (p_b.m_Data8b[1] != 0) &&
							(p_a.m_Data8b[2] != 0) == (p_b.m_Data8b[2] != 0) &&
							(p_a.m_Data8b[3] != 0) == (p_b.m_Data8b[3] != 0);
			}
		default:
			return memcmp(&p_a, &p_b, sizeof(uint32_t) * p_dimension) == 0;
	}
}

void PKAttributeList::resolve_effect_change() {
	const SAttributesContainer *default_attributes = *(effect->get_effect()->AttributeFlatList()->DefaultAttributes());
	PackedByteArray new_attribute_data;
	const int byte_count = default_attributes->Attributes().CoveredBytes();
	new_attribute_data.resize(byte_count);
	memcpy(new_attribute_data.ptrw(), default_attributes->Attributes().Data(), byte_count);
	SAttributesContainer_SAttrib *new_attributes = reinterpret_cast<SAttributesContainer_SAttrib *>(new_attribute_data.ptrw());

	for (int i = 0; i < effect->get_effect()->GetAttributeCount(); i++) {
		const CParticleAttributeDeclaration *decl = effect->get_effect()->GetAttributeDecl(i);
		const Ref<PKAttributeDesc> old_desc = get_attribute_desc_by_uid(decl->UID());
		if (old_desc == nullptr) {
			continue;
		}
		const String name = old_desc->get_name();
		resolve_attribute_change(decl, old_desc, new_attributes[i]);
	}

	attribute_data = new_attribute_data;

	attribute_descs.clear();
	for (int i = 0; i < effect->get_effect()->GetAttributeCount(); i++) {
		attribute_descs.append(PKAttributeDesc::from_declaration(effect->get_effect()->GetAttributeDecl(i), i));
	}

	const int sampler_count = effect->get_effect()->GetAttributeSamplerCount();
	TypedArray<PKAttributeSamplerDesc> new_attribute_sampler_descs;
	TypedArray<PKAttributeSampler> new_attribute_samplers;
	new_attribute_sampler_descs.resize(sampler_count);
	new_attribute_samplers.resize(sampler_count);

	for (int i = 0; i < sampler_count; i++) {
		const CParticleAttributeSamplerDeclaration *new_decl = effect->get_effect()->GetAttributeSamplerDecl(i);
		Ref<PKAttributeSamplerDesc> new_desc = PKAttributeSamplerDesc::from_declaration(new_decl, i);
		new_attribute_sampler_descs[i] = new_desc;
		Ref<PKAttributeSamplerDesc> old_desc = get_attribute_sampler_desc_by_uid(new_desc->get_uid());
		if (old_desc.is_null()) {
			new_attribute_samplers[i] = create_default_sampler(new_decl);
			continue;
		}
		if (new_desc->get_type() == old_desc->get_type()) {
			new_attribute_samplers[i] = get_attribute_sampler(old_desc->get_index());
		} else {
			new_attribute_samplers[i] = create_default_sampler(new_decl);
		}
	}

	set_attribute_sampler_descs(new_attribute_sampler_descs);
	set_attribute_samplers(new_attribute_samplers);

	reapply_attributes();
}

void PKAttributeList::resolve_attribute_change(const CParticleAttributeDeclaration *p_decl, Ref<PKAttributeDesc> p_old_desc, SAttributesContainer_SAttrib &r_value) const {
	const CBaseTypeTraits &old_traits = CBaseTypeTraits::Traits(EBaseTypeID(p_old_desc->get_type()));
	const CBaseTypeTraits &new_traits = CBaseTypeTraits::Traits(p_decl->GetType());
	if (old_traits.ScalarType != new_traits.ScalarType) {
		return; // Cannot keep anything with incompatible types
	}

	const int common_dimension = Math::min(old_traits.VectorDimension, new_traits.VectorDimension);
	SAttributesContainer_SAttrib current_value = attribute_raw_data()[p_old_desc->get_index()];
	if (attribs_match(current_value, p_old_desc->default_value_as_attrib(), p_decl->GetType(), common_dimension)) {
		return; // Keep updated default value if attribute value was still the default
	}

	p_decl->ClampToRangeIFN(current_value);
	memcpy(&r_value, &current_value, sizeof(uint32_t) * common_dimension);
}

void PKAttributeList::reapply_attributes() {
	for (int i = 0; i < attribute_descs.size(); i++) {
		const Ref<PKAttributeDesc> desc = get_attribute_desc(i);
		emitter->effect_instance->SetRawAttribute(i, EBaseTypeID(desc->get_type()), &attribute_raw_data()[i]);
	}

	for (int i = 0; i < attribute_sampler_descs.size(); i++) {
		const Ref<PKAttributeSampler> sampler = get_attribute_sampler(i);
		if (sampler.is_null()) {
			continue;
		}
		emitter->effect_instance->SetAttributeSampler(i, sampler->get_sampler());
	}
}

bool PKAttributeList::get_attribute(const String &p_name, SAttributesContainer_SAttrib &r_value) const {
	CHECK_EFFECT_VALID(false);
	const CGuid id = effect->get_effect()->GetAttributeID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, false);
	if (_check_instance_valid()) {
		const Ref<PKAttributeDesc> desc = attribute_descs[id];
		return emitter->effect_instance->GetRawAttribute(id, EBaseTypeID(desc->get_type()), &r_value);
	}
	memcpy(&r_value, &attribute_raw_data()[id], sizeof(SAttributesContainer_SAttrib));
	return true;
}

bool PKAttributeList::get_attribute(uint32_t p_id, SAttributesContainer_SAttrib &r_value) const {
	CHECK_EFFECT_VALID(false);
	const Ref<PKAttributeDesc> desc = attribute_descs[p_id];
	if (emitter->effect_instance != nullptr) {
		return emitter->effect_instance->GetRawAttribute(p_id, EBaseTypeID(desc->get_type()), &r_value);
	}
	memcpy(&r_value, &attribute_raw_data()[p_id], sizeof(SAttributesContainer_SAttrib));
	return true;
}

Variant PKAttributeList::get_attribute_variant(const String &p_name) const {
	CHECK_EFFECT_VALID(false);
	const CGuid id = effect->get_effect()->GetAttributeID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, nullptr);
	ERR_FAIL_COND_V(attribute_descs.size() <= id, nullptr);
	const Ref<PKAttributeDesc> desc = attribute_descs[id];
	const EBaseTypeID type = EBaseTypeID(desc->get_type());
	const EDataSemantic semantic = EDataSemantic(desc->get_semantic());
	SAttributesContainer_SAttrib data{};
	if (emitter->effect_instance != nullptr) {
		const bool ok = emitter->effect_instance->GetRawAttribute(id, type, &data);
		ERR_FAIL_COND_V(!ok, nullptr);
	} else {
		memcpy(&data, &attribute_raw_data()[id], CBaseTypeTraits::Traits(type).Size);
	}

	switch (type) {
		case PopcornFX::BaseType_Bool:
			return data.m_Data8b[0];
		case PopcornFX::BaseType_Bool2:
			return Array({ Variant(data.m_Data8b[0]), Variant(data.m_Data8b[1]) });
		case PopcornFX::BaseType_Bool3:
			return Array({ Variant(data.m_Data8b[0]), Variant(data.m_Data8b[1]), Variant(data.m_Data8b[2]) });
		case PopcornFX::BaseType_Bool4:
			return Array({ Variant(data.m_Data8b[0]), Variant(data.m_Data8b[1]), Variant(data.m_Data8b[2]), Variant(data.m_Data8b[3]) });

		case PopcornFX::BaseType_I32:
			return data.m_Data32i[0];
		case PopcornFX::BaseType_Int2:
			return Vector2i(data.m_Data32i[0], data.m_Data32i[1]);
		case PopcornFX::BaseType_Int3:
			if (semantic == DataSemantic_Color) {
				return Color::from_rgba8(data.m_Data32i[0], data.m_Data32i[1], data.m_Data32i[2]);
			}
			return Vector3i(data.m_Data32i[0], data.m_Data32i[1], data.m_Data32i[2]);
		case PopcornFX::BaseType_Int4:
			if (semantic == DataSemantic_Color) {
				return Color::from_rgba8(data.m_Data32i[0], data.m_Data32i[1], data.m_Data32i[2], data.m_Data32i[3]);
			}
			return Vector4i(data.m_Data32i[0], data.m_Data32i[1], data.m_Data32i[2], data.m_Data32i[3]);

		case PopcornFX::BaseType_Float:
			return data.m_Data32f[0];
		case PopcornFX::BaseType_Float2:
			return Vector2(data.m_Data32f[0], data.m_Data32f[1]);
		case PopcornFX::BaseType_Float3:
			if (semantic == DataSemantic_Color) {
				return Color(data.m_Data32f[0], data.m_Data32f[1], data.m_Data32f[2]);
			}
			return Vector3(data.m_Data32f[0], data.m_Data32f[1], data.m_Data32f[2]);
		case PopcornFX::BaseType_Float4:
			if (semantic == DataSemantic_Color) {
				return Color(data.m_Data32f[0], data.m_Data32f[1], data.m_Data32f[2], data.m_Data32f[3]);
			}
			return Vector4(data.m_Data32f[0], data.m_Data32f[1], data.m_Data32f[2], data.m_Data32f[3]);

		default:
			ERR_FAIL_V_MSG(false, vformat("Unsupported attribute type %s", CBaseTypeTraits::Traits(type).Name));
	}
}

bool PKAttributeList::set_attribute(const String &p_name, const SAttributesContainer_SAttrib &p_value) {
	CHECK_EFFECT_VALID(false);
	const CGuid id = effect->get_effect()->GetAttributeID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, false);
	ERR_FAIL_COND_V(attribute_descs.size() <= id, false);
	const Ref<PKAttributeDesc> desc = attribute_descs[id];
	if (emitter->effect_instance != nullptr && !emitter->effect_instance->SetRawAttribute(id, EBaseTypeID(desc->get_type()), &p_value)) {
		return false;
	}
	attribute_raw_data()[id] = p_value;
	return true;
}

bool PKAttributeList::set_attribute(uint32_t p_id, const SAttributesContainer_SAttrib &p_value) {
	ERR_FAIL_COND_V(emitter == nullptr, false);
	const Ref<PKAttributeDesc> desc = attribute_descs[p_id];
	if (emitter->effect_instance != nullptr && !emitter->effect_instance->SetRawAttribute(p_id, EBaseTypeID(desc->get_type()), &p_value)) {
		return false;
	}
	attribute_raw_data()[p_id] = p_value;
	return true;
}

bool PKAttributeList::set_attribute_variant(const String &p_name, const Variant &p_value) {
	CHECK_EFFECT_VALID(false);
	const CGuid id = effect->get_effect()->GetAttributeID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, false);
	ERR_FAIL_COND_V(attribute_descs.size() <= id, false);

	const Ref<PKAttributeDesc> desc = attribute_descs[id];
	const EBaseTypeID type = EBaseTypeID(desc->get_type());
	const EDataSemantic semantic = EDataSemantic(desc->get_semantic());
	ERR_FAIL_COND_V_MSG(!_check_type_matches(p_value, type), false,
			vformat("Mismatched types when trying to set attribute %s. Given values were: %s", p_name.utf8().ptr(), p_value));

	SAttributesContainer_SAttrib data{};

	switch (type) {
		case PopcornFX::BaseType_Bool:
			data.m_Data8b[0] = bool(p_value);
			break;
		case PopcornFX::BaseType_Bool2: {
			const Array array = Array(p_value);
			data.m_Data8b[0] = bool(array[0]);
			data.m_Data8b[1] = bool(array[1]);
			break;
		}
		case PopcornFX::BaseType_Bool3: {
			const Array array = Array(p_value);
			data.m_Data8b[0] = bool(array[0]);
			data.m_Data8b[1] = bool(array[1]);
			data.m_Data8b[2] = bool(array[2]);
			break;
		}
		case PopcornFX::BaseType_Bool4: {
			const Array array = Array(p_value);
			data.m_Data8b[0] = bool(array[0]);
			data.m_Data8b[1] = bool(array[1]);
			data.m_Data8b[2] = bool(array[2]);
			data.m_Data8b[3] = bool(array[3]);
			break;
		}

		case PopcornFX::BaseType_I32:
			data.m_Data32i[0] = int(p_value);
			break;
		case PopcornFX::BaseType_Int2: {
			const Vector2i vec = Vector2i(p_value);
			data.m_Data32i[0] = vec[0];
			data.m_Data32i[1] = vec[1];
			break;
		}
		case PopcornFX::BaseType_Int3: {
			if (semantic == DataSemantic_Color) {
				const Color color = Color(p_value);
				data.m_Data32i[0] = color.get_r8();
				data.m_Data32i[1] = color.get_g8();
				data.m_Data32i[2] = color.get_b8();
			} else {
				const Vector3i vec = Vector3i(p_value);
				data.m_Data32i[0] = vec[0];
				data.m_Data32i[1] = vec[1];
				data.m_Data32i[2] = vec[2];
			}
			break;
		}
		case PopcornFX::BaseType_Int4: {
			if (semantic == DataSemantic_Color) {
				const Color color = Color(p_value);
				data.m_Data32i[0] = color.get_r8();
				data.m_Data32i[1] = color.get_g8();
				data.m_Data32i[2] = color.get_b8();
				data.m_Data32i[3] = color.get_a8();
			} else {
				const Vector3i vec = Vector3i(p_value);
				data.m_Data32i[0] = vec[0];
				data.m_Data32i[1] = vec[1];
				data.m_Data32i[2] = vec[2];
				data.m_Data32i[3] = vec[3];
			}
			break;
		}

		case PopcornFX::BaseType_Float:
			data.m_Data32f[0] = float(p_value);
			break;
		case PopcornFX::BaseType_Float2: {
			const Vector2 vec = Vector2(p_value);
			data.m_Data32f[0] = vec[0];
			data.m_Data32f[1] = vec[1];
			break;
		}
		case PopcornFX::BaseType_Float3: {
			if (semantic == DataSemantic_Color) {
				const Color color = Color(p_value);
				data.m_Data32f[0] = color[0];
				data.m_Data32f[1] = color[1];
				data.m_Data32f[2] = color[2];
			} else {
				const Vector3 vec = Vector3(p_value);
				data.m_Data32f[0] = vec[0];
				data.m_Data32f[1] = vec[1];
				data.m_Data32f[2] = vec[2];
			}
			break;
		}
		case PopcornFX::BaseType_Float4: {
			if (semantic == DataSemantic_Color) {
				const Color color = Color(p_value);
				data.m_Data32f[0] = color[0];
				data.m_Data32f[1] = color[1];
				data.m_Data32f[2] = color[2];
				data.m_Data32f[3] = color[3];
			} else {
				const Vector4 vec = Vector4(p_value);
				data.m_Data32f[0] = vec[0];
				data.m_Data32f[1] = vec[1];
				data.m_Data32f[2] = vec[2];
				data.m_Data32f[3] = vec[3];
			}
			break;
		}

		default:
			ERR_FAIL_V_MSG(false, vformat("Unsupported attribute type %s", CBaseTypeTraits::Traits(type).Name));
	}

	if (emitter->effect_instance != nullptr &&
			!emitter->effect_instance->SetRawAttribute(id, type, &data)) {
		return false;
	}
	attribute_raw_data()[id] = data;
	return true;
}

Ref<PKAttributeSampler> PKAttributeList::create_default_sampler(const CParticleAttributeSamplerDeclaration *p_decl) {
	const PopcornFX::CResourceDescriptor *resource_descriptor = p_decl->AttribSamplerDefaultValue().Get();
	switch (p_decl->ExportedType()) {
		case SParticleDeclaration::SSampler::Sampler_Geometry:
			return memnew(PKAttributeSamplerShape(resource_descriptor));
		case SParticleDeclaration::SSampler::Sampler_Audio:
			return memnew(PKAttributeSamplerAudio(resource_descriptor));
		case SParticleDeclaration::SSampler::Sampler_Image:
			return memnew(PKAttributeSamplerImage(resource_descriptor));
		default:
			return {};
	}
}

const Ref<PKAttributeSampler> PKAttributeList::get_attribute_sampler(uint32_t p_id) const {
	ERR_FAIL_COND_V(emitter == nullptr, nullptr);
	ERR_FAIL_COND_V(attribute_samplers.size() <= p_id, nullptr);

	Variant sampler = attribute_samplers[p_id];
	if (sampler.get_type() == Variant::NIL) {
		return nullptr;
	}
	return sampler;
}

const Ref<PKAttributeSampler> PKAttributeList::get_attribute_sampler(const String &p_name) const {
	CHECK_EFFECT_VALID(nullptr);
	const CGuid id = effect->get_effect()->GetAttributeSamplerID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, nullptr);
	return get_attribute_sampler(id);
}

bool PKAttributeList::set_attribute_sampler(const String &p_name, Ref<PKAttributeSampler> p_sampler) {
	ERR_FAIL_COND_V(emitter == nullptr, false);
	const CGuid id = effect->get_effect()->GetAttributeSamplerID(p_name.utf8().ptr());
	ERR_FAIL_COND_V(id == CGuid::INVALID, false);
	ERR_FAIL_COND_V(attribute_samplers.size() <= id, false);
	const Ref<PKAttributeSamplerDesc> desc = attribute_sampler_descs[id];
	ERR_FAIL_NULL_V_MSG(desc, false, vformat("%s is not a valid attribute sampler name", p_name));
	return set_attribute_sampler(id, p_sampler);
}

bool PKAttributeList::set_attribute_sampler(uint32_t p_id, Ref<PKAttributeSampler> p_sampler) {
	CHECK_EFFECT_VALID(false);
	ERR_FAIL_COND_V(attribute_samplers.size() <= p_id, false);
	const Ref<PKAttributeSamplerDesc> desc = attribute_sampler_descs[p_id];
	ERR_FAIL_NULL_V_MSG(desc, false, vformat("%d is not a valid attribute sampler id", p_id));
	if (emitter->effect_instance != nullptr && !emitter->effect_instance->SetAttributeSampler(p_id, p_sampler.is_valid() ? p_sampler->get_sampler() : nullptr)) {
		return false;
	}
	return set_attribute_sampler_raw(p_id, p_sampler);
}

bool PKAttributeList::set_attribute_sampler_raw(uint32_t p_id, Ref<PKAttributeSampler> p_sampler) {
	attribute_samplers[p_id] = p_sampler;
	if (p_sampler.is_null()) {
		return true;
	}
	const Callable on_change = callable_mp(this, static_cast<bool (PKAttributeList::*)(uint32_t, Ref<PKAttributeSampler>)>(&PKAttributeList::set_attribute_sampler)).bind(p_sampler).bind(p_id);
	if (!p_sampler->is_connected("changed", on_change)) {
		p_sampler->connect("changed", on_change);
		p_sampler->connect("changed", callable_mp(static_cast<Resource *>(this), &PKAttributeList::emit_changed));
	}
	return true;
}

bool PKAttributeList::_check_type_matches(const Variant &p_variant, EBaseTypeID p_type) {
	const CBaseTypeTraits &traits = CBaseTypeTraits::Traits(p_type);
	const EBaseTypeID scalar_type = traits.ScalarType;
	const PopcornFX::u32 dimension = traits.VectorDimension;
	Variant::Type variant_type = p_variant.get_type();

	if (dimension > 1) {
		switch (variant_type) {
			case Variant::VECTOR2I:
				return dimension == 2 && scalar_type == PopcornFX::BaseType_I32;
			case Variant::VECTOR3I:
				return dimension == 3 && scalar_type == PopcornFX::BaseType_I32;
			case Variant::VECTOR4I:
				return dimension == 4 && scalar_type == PopcornFX::BaseType_I32;
			case Variant::VECTOR2:
				return dimension == 2 && scalar_type == PopcornFX::BaseType_Float;
			case Variant::VECTOR3:
				return dimension == 3 && scalar_type == PopcornFX::BaseType_Float;
			case Variant::VECTOR4:
				return dimension == 4 && scalar_type == PopcornFX::BaseType_Float;
			case Variant::COLOR:
				return (dimension == 3 || dimension == 4) && (scalar_type == PopcornFX::BaseType_Float || scalar_type == PopcornFX::BaseType_I32);
			case Variant::ARRAY:
				break;
			default:
				PK_ASSERT_NOT_REACHED(); // TODO change that to a PKGD_ASSERT_etc when these macros get merged along with the sound renderers
		}
		const Array as_array = Array(p_variant);
		if (as_array.size() != dimension) {
			return false;
		}
		const Variant::Type array_type = as_array[0].get_type();
		for (const Variant &elem : as_array) {
			if (elem.get_type() != array_type) {
				return false;
			}
		}
		variant_type = array_type;
	}

	switch (scalar_type) {
		case PopcornFX::BaseType_Bool:
			return variant_type == Variant::BOOL;
		case PopcornFX::BaseType_I32:
			return variant_type == Variant::INT;
		case PopcornFX::BaseType_Float:
			return variant_type == Variant::FLOAT;
		default:
			ERR_FAIL_V_MSG(false, vformat("Unsupported attribute type %s", CBaseTypeTraits::Traits(p_type).Name));
	}
}

bool PKAttributeList::_check_effect_valid() const {
	ERR_FAIL_NULL_V(emitter, false);
	ERR_FAIL_NULL_V(effect, false);
	if (effect->get_effect() == nullptr) { // Can happen when reimporting, no err_fail
		return false;
	}

	return true;
}

bool PKAttributeList::_check_instance_valid() const {
	return emitter->get_effect_instance() != nullptr && emitter->get_effect_instance()->Alive();
}
} // namespace godot
