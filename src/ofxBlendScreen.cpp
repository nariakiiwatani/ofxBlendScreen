#include "ofxBlendScreen.h"
#include <glm/vec2.hpp>
#include "ofTexture.h"

using namespace geom;

namespace ofxBlendScreen {
ofMesh createMesh(const Quad &vertex_outer, const Quad &vertex_inner, const Quad &vertex_frame, const Quad &texture_uv_for_frame, int blend_flag)
{
	Quad texture_uv_for_outer;
	for(int i = 0; i < 4; ++i) {
		remapPosition(vertex_frame, texture_uv_for_frame, vertex_outer[i], texture_uv_for_outer[i]);
	}
	return createMesh(vertex_outer, vertex_inner, texture_uv_for_outer, blend_flag);
}
ofMesh createMesh(const Quad &vertex_outer, const Quad &vertex_inner, const Quad &texture_uv, int blend_flag)
{
	ofMesh ret;
	ret.getVertices() = std::vector<glm::vec3>{
		{vertex_outer.lt, 0},
		{intersection(vertex_outer.lt, vertex_outer.rt, vertex_inner.lt, vertex_inner.lb), 0},
		{intersection(vertex_outer.rt, vertex_outer.lt, vertex_inner.rt, vertex_inner.rb), 0},
		{vertex_outer.rt, 0},
		{intersection(vertex_outer.lt, vertex_outer.lb, vertex_inner.lt, vertex_inner.rt), 0},
		{vertex_inner.lt, 0},
		{vertex_inner.rt, 0},
		{intersection(vertex_outer.rt, vertex_outer.rb, vertex_inner.lt, vertex_inner.rt), 0},
		{intersection(vertex_outer.lb, vertex_outer.lt, vertex_inner.lb, vertex_inner.rb), 0},
		{vertex_inner.lb, 0},
		{vertex_inner.rb, 0},
		{intersection(vertex_outer.rb, vertex_outer.rt, vertex_inner.lb, vertex_inner.rb), 0},
		{vertex_outer.lb, 0},
		{intersection(vertex_outer.lb, vertex_outer.rb, vertex_inner.lt, vertex_inner.lb), 0},
		{intersection(vertex_outer.rb, vertex_outer.lb, vertex_inner.rt, vertex_inner.rb), 0},
		{vertex_outer.rb, 0},
	};
	auto &texcoords = ret.getTexCoords();
	for(auto &&v : ret.getVertices()) {
		glm::vec2 uv;
		remapPosition(vertex_outer, texture_uv, v, uv);
		texcoords.emplace_back(uv);
	}
	float power_l = (blend_flag&BLEND_LEFT) == 0;
	float power_r = (blend_flag&BLEND_RIGHT) == 0;
	float power_t = (blend_flag&BLEND_TOP) == 0;
	float power_b = (blend_flag&BLEND_BOTTOM) == 0;
	float power_1 = 1;
	const auto blend_power = std::vector<glm::vec2>{ 
		{power_l,power_t},{power_1,power_t},{power_1,power_t},{power_r,power_t},
		{power_l,power_1},{power_1,power_1},{power_1,power_1},{power_r,power_1},
		{power_l,power_1},{power_1,power_1},{power_1,power_1},{power_r,power_1},
		{power_l,power_b},{power_1,power_b},{power_1,power_b},{power_r,power_b},
	};
	auto &colors = ret.getColors();
	colors.clear();
	for(auto &&p : blend_power) {
		colors.emplace_back(p.x,p.y,0,1);
	}

	auto createIndices = [](int iteration, int stride, const std::vector<ofIndexType> &base) {
		std::vector<ofIndexType> ret;
		for(int i = 0; i < iteration; ++i) {
			for(auto &&b : base) {
				ret.push_back(b+stride*i);
			}
		}
		return ret;
	};

	ret.getIndices() = createIndices(3,4,createIndices(3,1,{0,4,1,1,4,5}));
	ret.setMode(OF_PRIMITIVE_TRIANGLES);
	
	return ret;
}

void Shader::setup() {
	shader_.setupShaderFromSource(GL_VERTEX_SHADER, R"(#version 410

uniform mat4 modelViewProjectionMatrix;
layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=3) in vec2 texcoord;

out vec2 texCoordVarying;
out vec2 blendLevelVarying;

void main() {
	gl_Position = modelViewProjectionMatrix * position;
	texCoordVarying = texcoord;
	blendLevelVarying = color.rg;
}
)");
	shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, R"(#version 410

uniform sampler2D tex;
uniform vec3 gamma;
uniform float blendPower;
uniform float luminanceControl;
uniform vec3 baseColor;
in vec2 blendLevelVarying;
in vec2 texCoordVarying;
out vec4 outputColor;

vec4 blend(float alpha, vec3 gamma, float blendPower, float luminanceControl) {
	alpha = (alpha < 0.5)
			? luminanceControl * pow(2.0 * alpha, blendPower) 
			: 1.0 - (1.0 - luminanceControl) * pow(2.0 * (1.0 - alpha), blendPower);

	return vec4(pow(alpha, 1.0 / gamma.r),
				pow(alpha, 1.0 / gamma.g),
				pow(alpha, 1.0 / gamma.b),
				1.0);
}

void main () {
	outputColor = texture(tex, texCoordVarying);
	if(blendLevelVarying.x < 1) {
	 outputColor *= blend(blendLevelVarying.x, gamma, blendPower, luminanceControl);
	}
	if(blendLevelVarying.y < 1) {
	 outputColor *= blend(blendLevelVarying.y, gamma, blendPower, luminanceControl);
	}
	if(blendLevelVarying.x*blendLevelVarying.y >= 0.9999) {
		outputColor += vec4(baseColor, 1.0);
	}
}
)");
	shader_.linkProgram();
}
void Shader::begin(ofTexture tex)
{
	shader_.begin();
	shader_.setUniformTexture("tex", tex, 0);
	shader_.setUniform3f("gamma", params_.gamma);
	shader_.setUniform1f("luminanceControl", params_.luminance_control);
	shader_.setUniform1f("blendPower", params_.blend_power);
	shader_.setUniform3f("baseColor", params_.base_color);
}
void Shader::end() {
	shader_.end();
}
}
