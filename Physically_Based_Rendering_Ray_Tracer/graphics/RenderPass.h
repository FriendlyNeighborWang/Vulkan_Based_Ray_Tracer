#ifndef PBRT_GRAPHICS_RENDER_PASS_H
#define PBRT_GRAPHICS_RENDER_PASS_H

#include "base/base.h"
#include "util/pstd.h"

#include <functional>

class RenderPass {
public:
	using ExecuteFn = std::function<void(FrameContext&)>;

	RenderPass(const std::string& name, ExecuteFn fn);


	const std::string& get_name() const { return _name; }

private:
	std::string _name;
	
	std::unordered_map<std::string, Pipeline*> pipelines;

	ExecuteFn _execute_fn;

};

#endif // !PBRT_GRAPHICS_RENDER_PASS_H

