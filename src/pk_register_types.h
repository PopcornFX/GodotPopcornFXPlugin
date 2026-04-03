//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define DEFINE_BASIC_PROPERTY(type, name) \
private:                                  \
	type name;                            \
                                          \
public:                                   \
	void set_##name(type name) {          \
		this->name = name;                \
	}                                     \
	type get_##name() const {             \
		return name;                      \
	}
#define BIND_BASIC_PROPERTY(bind_type, type, name, ...)                             \
	ClassDB::bind_method(D_METHOD("_set_" #name, #name), &bind_type::set_##name); \
	ClassDB::bind_method(D_METHOD("_get_" #name), &bind_type::get_##name);        \
	ADD_PROPERTY(PropertyInfo(Variant::type, #name, __VA_ARGS__), "_set_" #name, "_get_" #name);
