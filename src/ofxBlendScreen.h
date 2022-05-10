#pragma once

#include "ofShader.h"
#include "ofMesh.h"
#include "ofRectangle.h"
#include "Quad.h"

namespace ofxBlendScreen
{
enum {
	BLEND_LEFT=1<<0,
	BLEND_RIGHT=1<<1,
	BLEND_TOP=1<<2,
	BLEND_BOTTOM=1<<3,
	BLEND_ALL = ~0
};
ofMesh createMesh(const geom::Quad &vertex_outer, const geom::Quad &vertex_inner, const geom::Quad &texture_uv_for_outer, int blend_flag=BLEND_ALL);
ofMesh createMesh(const geom::Quad &vertex_outer, const geom::Quad &vertex_inner, const geom::Quad &vertex_frame, const geom::Quad &texture_uv_for_frame, int blend_flag=BLEND_ALL);
class Shader
{
public:
	struct Params {
		glm::vec3 gamma=glm::vec3{1,1,1};
		float luminance_control=0.5f;
		float blend_power=2;
		glm::vec3 base_color=glm::vec3{0,0,0};
	};
	void setup();
	void begin(ofTexture tex);
	Params& getParams() { return params_; }
	void end();
private:
	ofShader shader_;
	Params params_;
};
}
