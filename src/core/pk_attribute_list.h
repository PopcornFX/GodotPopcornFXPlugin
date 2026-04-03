//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/core/object.hpp"

#include "core/pk_effect.h"
#include "core/samplers/pk_attribute_sampler.h"
#include "integration/pk_sdk.h"
#include "pk_register_types.h"

#include <pk_particles/include/ps_attributes.h>

namespace godot {

class PKEmitter3D;

class PKAttributeDesc : public Resource {
	GDCLASS(PKAttributeDesc, Resource);

public:
	PKAttributeDesc();

	DEFINE_BASIC_PROPERTY(String, name)
	DEFINE_BASIC_PROPERTY(int, index)
	DEFINE_BASIC_PROPERTY(int, type)
	DEFINE_BASIC_PROPERTY(int, uid)
	DEFINE_BASIC_PROPERTY(PackedByteArray, default_value)

	static PKAttributeDesc *from_declaration(const CParticleAttributeDeclaration *p_declaration, int p_index);
	SAttributesContainer_SAttrib default_value_as_attrib() const;

protected:
	static void _bind_methods();
};

class PKAttributeSamplerDesc : public Resource {
	GDCLASS(PKAttributeSamplerDesc, Resource);

public:
	PKAttributeSamplerDesc();

	DEFINE_BASIC_PROPERTY(String, name)
	DEFINE_BASIC_PROPERTY(int, index)
	DEFINE_BASIC_PROPERTY(int, type)
	DEFINE_BASIC_PROPERTY(int, uid)

	static PKAttributeSamplerDesc *from_declaration(const CParticleAttributeSamplerDeclaration *p_declaration, int p_index);

protected:
	static void _bind_methods();
};

class PKAttributeList : public Resource {
	GDCLASS(PKAttributeList, Resource);
	friend class PKEmitter3D;

public:
	PKAttributeList();
	static Ref<PKAttributeList> default_for_emitter(PKEmitter3D *p_emitter);

	DEFINE_BASIC_PROPERTY(Ref<PKEffect>, effect)
	DEFINE_BASIC_PROPERTY(TypedArray<PKAttributeDesc>, attribute_descs)
	DEFINE_BASIC_PROPERTY(PackedByteArray, attribute_data)
	DEFINE_BASIC_PROPERTY(TypedArray<PKAttributeSamplerDesc>, attribute_sampler_descs)

	TypedArray<PKAttributeSampler> attribute_samplers;
	PKEmitter3D *emitter;

	void set_emitter(PKEmitter3D *p_emitter);
	void set_attribute_samplers(TypedArray<PKAttributeSampler> p_attribute_samplers);
	TypedArray<PKAttributeSampler> get_attribute_samplers() const {
		return attribute_samplers;
	}

	SAttributesContainer_SAttrib *attribute_raw_data();
	const SAttributesContainer_SAttrib *attribute_raw_data() const;

	Ref<PKAttributeDesc> get_attribute_desc(uint32_t p_id);
	Ref<PKAttributeSamplerDesc> get_attribute_sampler_desc(uint32_t p_id);
	Ref<PKAttributeDesc> get_attribute_desc(const char *p_name);
	Ref<PKAttributeSamplerDesc> get_attribute_sampler_desc(const char *p_name);
	Ref<PKAttributeDesc> get_attribute_desc_by_uid(uint32_t p_uid);
	Ref<PKAttributeSamplerDesc> get_attribute_sampler_desc_by_uid(uint32_t p_uid);

	void resolve_effect_change();
	void resolve_attribute_change(const CParticleAttributeDeclaration *p_decl, Ref<PKAttributeDesc> p_old_desc, SAttributesContainer_SAttrib &r_value) const;
	void reapply_attributes();
	bool get_attribute(const char *p_name, SAttributesContainer_SAttrib &r_value) const;
	bool get_attribute(uint32_t p_id, SAttributesContainer_SAttrib &r_value) const;
	bool set_attribute(const char *p_name, const SAttributesContainer_SAttrib &p_value);
	bool set_attribute(uint32_t p_id, const SAttributesContainer_SAttrib &p_value);

	Ref<PKAttributeSampler> create_default_sampler(const CParticleAttributeSamplerDeclaration *p_decl) const;

	const Ref<PKAttributeSampler> get_attribute_sampler(uint32_t p_id) const;
	template <typename S>
	const Ref<S> get_attribute_sampler_or_default(uint32_t p_id) {
		static_assert(std::is_base_of_v<PKAttributeSampler, S>, "get_attribute_sampler_or_default can only be used with PKAttributeSampler subclasses.");
		ERR_FAIL_COND_V(emitter == nullptr, nullptr);
		ERR_FAIL_COND_V(attribute_samplers.size() <= p_id, nullptr);

		Ref<PKAttributeSampler> sampler = get_attribute_sampler(p_id);
		if (sampler == nullptr) {
			const PopcornFX::CResourceDescriptor *resource_desc = all_attribute_sampler_declarations()[p_id]->AttribSamplerDefaultValue().Get();
			sampler = Ref<PKAttributeSampler>(memnew(S(resource_desc)));
			sampler->set_emitter(emitter);

			set_attribute_sampler(p_id, sampler);
			return sampler;
		}
		ERR_FAIL_COND_V_MSG(!sampler->is_class(S::get_class_static()), nullptr, vformat("Expected type %s for Attribute Sampler %d, got %s", S::get_class_static(), p_id, sampler->get_class()));
		return sampler;
	}
	bool set_attribute_sampler(const char *p_name, Ref<PKAttributeSampler> p_sampler);
	bool set_attribute_sampler(uint32_t p_id, Ref<PKAttributeSampler> p_sampler);
	bool set_attribute_sampler_raw(uint32_t p_id, Ref<PKAttributeSampler> p_sampler, TypedArray<PKAttributeSampler> &p_samplers);

	TMemoryView<CParticleAttributeDeclaration *const> all_attribute_declarations() const;
	TMemoryView<CParticleAttributeSamplerDeclaration *const> all_attribute_sampler_declarations() const;

protected:
	static void _bind_methods();
	void _physics_process();
	void _ready();
};

} // namespace godot
