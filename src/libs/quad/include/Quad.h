#pragma once

#include <glm/vec2.hpp>
#include "ofRectangle.h"
#include "ofMath.h"
#include <numeric>

namespace geom {
struct Quad {
	using value_type = glm::vec2;
	Quad() { lt = {0,0}; rt = {1,0}; lb = {0,1}; rb = {1,1}; }
	Quad(value_type lt, value_type rt, value_type lb, value_type rb):Quad() { this->lt = lt; this->rt = rt; this->lb = lb; this->rb = rb; }
	Quad(const value_type pt[4]):Quad(pt[0],pt[1],pt[2],pt[3]) {}
	Quad(const ofRectangle &rect):Quad(rect.getTopLeft(), rect.getTopRight(), rect.getBottomLeft(), rect.getBottomRight()) {}
	Quad(float x, float y, float width, float height):Quad(ofRectangle(x,y,width,height)){}
	Quad(float width, float height):Quad(0,0,width,height){}
	Quad(float size):Quad(size,size){}
	Quad(const Quad &q):Quad(q.pt){}
	Quad(Quad &&q):Quad(q.pt){}
	Quad& operator=(const Quad &q) { lt = q.lt; rt = q.rt; lb = q.lb; rb = q.rb; return *this; }
	Quad& operator=(Quad &&q) { lt = q.lt; rt = q.rt; lb = q.lb; rb = q.rb; return *this; }
	value_type& operator[](std::size_t index) { return pt[index]; }
	const value_type& operator[](std::size_t index) const { return pt[index]; }
	std::size_t size() const { return 4; }
	value_type* begin() { return pt; }
	value_type* end() { return pt+size(); }
	const value_type* begin() const { return pt; }
	const value_type* end() const { return pt+size(); }
	union {
		value_type pt[4];
		struct {
			value_type lt, rt, lb, rb;
		};
	};
};
static glm::vec2 intersection(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d) {
	// Line AB represented as a1x + b1y = c1 
	double a1 = b.y - a.y; 
	double b1 = a.x - b.x; 
	double c1 = a1*(a.x) + b1*(a.y); 

	// Line CD represented as a2x + b2y = c2 
	double a2 = d.y- c.y; 
	double b2 = c.x - d.x; 
	double c2 = a2*(c.x)+ b2*(c.y); 

	double determinant = a1*b2 - a2*b1; 

	if (determinant == 0) 
	{ 
		// The lines are parallel. 
		return a; 
	} 
	else
	{ 
		double x = (b2*c1 - b1*c2)/determinant; 
		double y = (a1*c2 - a2*c1)/determinant; 
		return {x, y}; 
	}
}
static bool inside(const Quad &quad, glm::vec2 point) {
	// https://www.nttpc.co.jp/technology/number_algorithm.html
	bool ret = false;
	int indices[] = {0,1,3,2,0};
	for(int i = 0; i < quad.size(); ++i) {
		auto p0 = quad[indices[i]], p1 = quad[indices[i+1]];
		if((p0.y <= point.y && p1.y > point.y)
		   || (p0.y > point.y && p1.y <= point.y)) {
			auto vt = (point.y - p0.y) / (p1.y - p0.y);
			if(point.x < p0.x + vt * (p1.x - p0.x)) {
				ret ^= true;
			}
		}
	}
	return ret;
}
static glm::vec2 getCenterOfGravity(const Quad &quad) {
	return std::accumulate(std::begin(quad), std::end(quad), Quad::value_type(), [](Quad::value_type sum, const Quad::value_type &p) {
		return sum+p;
	})/(float)quad.size();
}
static float getDistanceFromCenterOfGravity(const Quad &quad, glm::vec2 pos) {
	return distance(getCenterOfGravity(quad), pos);
}
static bool normalizedPosition(const Quad &quad, glm::vec2 scaled, glm::vec2 &result) {
	glm::vec2 AB = quad.rt-quad.lt;
	glm::vec2 AC = quad.rb-quad.lt;
	glm::vec2 AD = quad.lb-quad.lt;
	glm::vec2 AP = scaled-quad.lt;
	glm::vec2 CDB = AC-AD-AB;
	auto calc_t = [=](float s) {
		float div = AD.y+s*CDB.y;
		return div == 0 ? 0 : (AP.y-s*AB.y)/div;
	};
	float a = AB.y*CDB.x - AB.x*CDB.y;
	float b = (AB.y*AD.x - AP.y*CDB.x) - (AB.x*AD.y - AP.x*CDB.y);
	float c = AP.x*AD.y - AP.y*AD.x;
	float D = b*b - 4*a*c;
	if(D < 0) {
//			ofLogWarning(LOG_TITLE) << "not found";
		return false;
	}
	if(a == 0) {
		float s = b == 0 ? 0 : -c/b;
		float t = calc_t(s);
		result = {s, t};
		return ofInRange(s,0,1) && ofInRange(t,0,1);
	}
	float sqrtD = sqrt(D);
	float s = (-b+sqrtD)/(2*a);
	float t = calc_t(s);
	if(ofInRange(s,0,1) && ofInRange(t,0,1)) {
		result = {s, t};
		return true;
	}
	s = (-b-sqrtD)/(2*a);
	t = calc_t(s);
	result = {s, t};
	return ofInRange(s,0,1) && ofInRange(t,0,1);
}
static glm::vec2 rescalePosition(const Quad &quad, glm::vec2 normalized) {
	glm::vec2 AB = quad.rt-quad.lt;
	glm::vec2 AC = quad.rb-quad.lt;
	glm::vec2 AD = quad.lb-quad.lt;
	float s = normalized.x, t = normalized.y;
	return quad.lt+s*AB+t*(AD+s*t*(AC-AB-AD));
}
static bool remapPosition(const Quad &src_space, const Quad &dst_space, glm::vec2 src, glm::vec2 &dst) {
	if(!normalizedPosition(src_space, src, dst)) {
		return false;
	}
	dst = rescalePosition(dst_space, dst);
	return true;
}
static bool remapPosition(const Quad &src_space, const Quad &dst_space, glm::vec2 &src_dst) {
	return remapPosition(src_space, dst_space, src_dst, src_dst);
}
static Quad getScaled(const Quad &src, glm::vec2 scale) {
	Quad ret(src);
	for(int i = 0; i < ret.size(); ++i) {
		ret[i] *= scale;
	}
	return ret;
}
static void scale(Quad &src_dst, glm::vec2 scale) {
	src_dst = getScaled(src_dst, scale);
}
static Quad getTranslated(const Quad &src, glm::vec2 trans) {
	Quad ret(src);
	for(int i = 0; i < ret.size(); ++i) {
		ret[i] += trans;
	}
	return ret;
}
static void translate(Quad &src_dst, glm::vec2 trans) {
	src_dst = getTranslated(src_dst, trans);
}
}
