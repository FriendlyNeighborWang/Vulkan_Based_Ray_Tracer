#ifndef GRAPHICS_GLSL_LAYOUT_INFO_H
#define GRAPHICS_GLSL_LAYOUT_INFO_H

#include <string>

struct GLSLLayoutInfo {
	std::string blockName;
	std::string memberType;
	std::string memberName;
	bool isArray = false;
};

#endif // !GRAPHICS_GLSL_LAYOUT_INFO_H

