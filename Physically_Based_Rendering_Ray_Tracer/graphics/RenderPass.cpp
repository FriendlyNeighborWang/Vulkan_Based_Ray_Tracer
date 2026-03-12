#include "RenderPass.h"

RenderPass::RenderPass(const std::string& name, ExecuteFn fn):_name(name), _execute_fn(std::move(fn)){}

