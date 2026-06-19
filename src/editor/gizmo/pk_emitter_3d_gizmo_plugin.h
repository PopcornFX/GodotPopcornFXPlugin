#pragma once

#include "godot_cpp/classes/editor_node3d_gizmo_plugin.hpp"
#include "godot_cpp/classes/node3d.hpp"

namespace godot {

class PKEmitter3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(PKEmitter3DGizmoPlugin, EditorNode3DGizmoPlugin);

public:
	PKEmitter3DGizmoPlugin();

	virtual bool _has_gizmo(Node3D *p_for_node_3d) const override;
	virtual String _get_gizmo_name() const override { return "PKEmitter3D"; }
	virtual void _redraw(const Ref<EditorNode3DGizmo> &p_gizmo) override;

protected:
	static void _bind_methods() {}

private:
	bool _try_load();
	bool loaded;
};

} //namespace godot
