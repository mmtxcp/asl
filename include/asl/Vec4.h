// Copyright(c) 1999-2018 ASL author
// Licensed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ASL_VECTOR4_H
#define ASL_VECTOR4_H

#include <asl/defs.h>
#include <asl/Vec3.h>
#include <math.h>

namespace asl {

/**
A Vec4 is a 4-dimensional vector usually representing homogenous coordinates.

It can be used together with class Matrix4 to transform vectors in projective space.

~~~
Vec4 a (10, 10, 0.1, 1.0);
Vec3 v = a.h2c();
~~~

*/

class Vec4
{
 public:
	Vec4() {}
	Vec4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {}
	Vec4(const Vec3& v, float W) : x(v.x), y(v.y), z(v.z), w(W) {}
	Vec4(const Vec4& v): x(v.x), y(v.y), z(v.z), w(v.w) {}
	Vec4(const float* v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
	operator const float*() const {return (float*)this;}
	/** Returns the *x*, *y*, *z* components as a Vec3 */
	Vec3 xyz() const {return Vec3(x, y, z);}
	/** Returns the cartesian coordinates vector corresponding to this homogenous coordinates */
	Vec3 h2c() const {float iw=1/w; return Vec3(iw*x, iw*y, iw*z);}

	/** Returns a normalized version of this vector */
	Vec4 normalized() const {
		return *this/length();
	}
	/** Returns the length of the vector */
	float length() const {return sqrt(x*x+y*y+z*z+w*w);}
	/** Returns the length of the vector squared */
	float length2() const {return x*x+y*y+z*z+w*w;}
	/** Returns the length of the vector */
	float operator!() const {return length();}
	Vec4 abs() const {return Vec4(fabs(x), fabs(y), fabs(z), fabs(w));}

	void operator=(const Vec4& b) {x = b.x; y = b.y; z = b.z; w = b.w;}
	/** Returns this plus `b` */
	Vec4 operator+(const Vec4& b) const {return Vec4(x+b.x, y+b.y, z+b.z, w+b.w);}
	/** Returns this minus `b` */
	Vec4 operator-(const Vec4& b) const {return Vec4(x-b.x, y-b.y, z-b.z, w-b.w);}
	/** Returns the *dot product* of this vector and `b` */
	float operator*(const Vec4& b) const {return x*b.x+y*b.y+z*b.z+w*b.w;}
	/** Returns this vector multiplied by scalar `r` */
	Vec4 operator*(float r) const {return Vec4(x*r, y*r, z*r, w*r);}
	/** Returns this vector multiplied by scalar `r` */
	friend Vec4 operator*(float r, const Vec4& b) {return b*r;}
	Vec4 operator/(float r) const {float t=1.0f/r; return Vec4(t*x, t*y, t*z, t*w);}
	/** Returns a vector that is a component-wise product of this vector and `b` */
	Vec4 operator%(const Vec4& b) const {return Vec4(x*b.x, y*b.y, z*b.z, w*b.w);}
	/** Checks if this vector is equal to `b` */
	bool operator==(const Vec4& b) const {return x==b.x && y==b.y && z==b.z && w==b.w;}
	/** Checks if this vector is not equal to `b` */
	bool operator!=(const Vec4& b) const {return x!=b.x || y!=b.y || z!=b.z || w!=b.w;}
	/** Adds vector `b` to this vector */
	void operator+=(const Vec4& b) {x += b.x; y += b.y; z += b.z; w += b.w;}
	/** Subtracts vector `b` from this vector */
	void operator-=(const Vec4& b) {x -= b.x; y -= b.y; z -= b.z; w -= b.w;}
	/** Multiplies this vector by scalar `r` */
	void operator*=(float r) {x *= r; y *= r; z *= r; w *= r;}
	/** Divides this vector by scalar `r` */
	void operator/=(float r) {float t=1.0f/r; x *= t; y *= t; z *= t; w *= t;}
	/** Returns this vector negated */
	Vec4 operator-() const {return Vec4(-x,-y,-z, -w);}

public:
	/** The x, y, z, w components */
	float x, y, z, w;
};

// CHECK

inline int compare(const Vec4& a, const Vec4& b)
{
	if(a.x < b.x) return -1;
	else if(a.x == b.x && a.y < b.y) return -1;
	else if(a.x == b.x && a.y == b.y && a.z < b.z) return -1;
	else if(a.x == b.x && a.y == b.y && a.z == b.z) return 0;
	else if(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w) return 0;
	else return 1;
}

}

#endif