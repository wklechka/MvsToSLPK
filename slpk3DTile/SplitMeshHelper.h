#pragma once
constexpr double Epsilon = 4.9406564584124654E-324 * 10.0;

class Vertex2
{
public:
	double X;
	double Y;

public:

	bool Equals(Vertex2& other)
	{
		return X == other.X && Y == other.Y;
	}

	Vertex2(double x, double y)
	{
		X = x;
		Y = y;
	}
	Vertex2()
	{
		X = 0.0;
		Y = 0.0;
	}

	bool operator ==(const Vertex2& b) const
	{
		return fabs(X - b.X) < Epsilon && fabs(Y - b.Y) < Epsilon;
	}

	bool operator !=(const Vertex2& b) const
	{
		return fabs(X - b.X) > Epsilon || fabs(Y - b.Y) > Epsilon;
	}

	double Distance(const Vertex2& other)
	{
		return sqrt((X - other.X) * (X - other.X) + (Y - other.Y) * (Y - other.Y));
	}

	Vertex2 cutEdgePerc(const Vertex2 b, double perc)
	{
		return Vertex2((b.X - X) * perc + X, (b.Y - Y) * perc + Y);
	}
};

template<>
struct std::hash<Vertex2>
{
	std::size_t operator()(const Vertex2& s) const noexcept
	{
		std::size_t h1 = std::hash<double>{}(s.X);
		std::size_t h2 = std::hash<double>{}(s.Y);
		return h1 ^ (h2 << 1); // or use boost::hash_combine
	}
};


class Vertex3
{
public:
	double X;
	double Y;
	double Z;

	Vertex3(double x, double y, double z)
	{
		X = x;
		Y = y;
		Z = z;
	}
	Vertex3()
	{
		X = 0.0;
		Y = 0.0;
		Z = 0.0;
	}

	bool Equals(Vertex3& other)
	{
		return X == other.X && Y == other.Y && Z == other.Z;
	}


	bool operator ==(const Vertex3& b) const
	{
		return fabs(X - b.X) < Epsilon && fabs(Y - b.Y) < Epsilon && fabs(Z - b.Z) < Epsilon;
	}

	bool operator !=(const Vertex3& b) const
	{
		return !(operator == (b));
	}

	double Distance(const Vertex3& other)
	{
		return sqrt((X - other.X) * (X - other.X) + (Y - other.Y) * (Y - other.Y) + (Z - other.Z) * (Z - other.Z));
	}

	Vertex3 operator +(Vertex3& b)
	{
		return Vertex3(X + X, Y + Y, Z + Z);
	}

	Vertex3 operator -(Vertex3& b)
	{
		return Vertex3(X - b.X, Y - b.Y, Z - b.Z);
	}

	Vertex3 operator *(double b)
	{
		return Vertex3(X * b, Y * b, Z * b);
	}

	Vertex3 operator /(double b)
	{
		return Vertex3(X / b, Y / b, Z / b);
	}

	Vertex3 Cross(Vertex3& other)
	{
		return Vertex3(Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X);
	}

	Vertex3 cutEdgePerc(double perc)
	{
		return Vertex3((X - X) * perc + X, (Y - Y) * perc + Y, (Z - Z) * perc + Z);
	}
};

template<>
struct std::hash<Vertex3>
{
	std::size_t operator()(const Vertex3& s) const noexcept
	{
		size_t h1 = std::hash<double>()(s.X);
		size_t h2 = std::hash<double>()(s.Y);
		size_t h3 = std::hash<double>()(s.Z);
		return (h1 ^ (h2 << 1)) ^ h3;
	}
};



class IVertexUtilsBase
{
public:
	enum Axis
	{
		X,
		Y,
		Z
	};

public:
	virtual Vertex3 cutEdge(Vertex3 a, Vertex3 b, double q) = 0;
	virtual double getDim(Vertex3 v) = 0;

	Axis axis;
};

class VertexUtilsX : public IVertexUtilsBase
{
public:

	VertexUtilsX() {
		axis = Axis::X;
	}

	virtual Vertex3 cutEdge(Vertex3 a, Vertex3 b, double q)
	{
		auto dx = a.X - b.X;
		auto my = (a.Y - b.Y) / dx;
		auto mz = (a.Z - b.Z) / dx;

		return Vertex3(q, my * (q - a.X) + a.Y, mz * (q - a.X) + a.Z);
	}

	virtual double getDim(Vertex3 v)
	{
		return v.X;
	}
};

class VertexUtilsY : public IVertexUtilsBase
{
public:
	VertexUtilsY() {
		axis = Axis::Y;
	}

	virtual Vertex3 cutEdge(Vertex3 a, Vertex3 b, double q)
	{
		auto dy = a.Y - b.Y;
		auto mx = (a.X - b.X) / dy;
		auto mz = (a.Z - b.Z) / dy;

		return Vertex3(mx * (q - a.Y) + a.X, q, mz * (q - a.Y) + a.Z);
	}

	virtual double getDim(Vertex3 v)
	{
		return v.Y;
	}
};

class VertexUtilsZ : public IVertexUtilsBase
{
public:
	VertexUtilsZ() {
		axis = Axis::Z;
	}

	virtual Vertex3 cutEdge(Vertex3 a, Vertex3 b, double q)
	{
		auto dz = a.Z - b.Z;
		auto mx = (a.X - b.X) / dz;
		auto my = (a.Y - b.Y) / dz;

		return Vertex3(mx * (q - a.Z) + a.X, my * (q - a.Z) + a.Y, q);
	}

	virtual double getDim(Vertex3 v)
	{
		return v.Z;
	}
};

class Box3
{
public:
	Vertex3 Min;
	Vertex3 Max;

	Box3()
	{
	}

	Box3(Vertex3 min, Vertex3 max)
	{
		Min = min;
		Max = max;
	}


	Box3(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
	{
		Min = Vertex3(minX, minY, minZ);
		Max = Vertex3(maxX, maxY, maxZ);
	}

	double width() {
		return Max.X - Min.X;
	}
	double height() {
		return Max.Y - Min.Y;
	}
	double depth() {
		return Max.Z - Min.Z;
	}
	Vertex3 center() {
		return Vertex3((Min.X + Max.X) / 2.0, (Min.Y + Max.Y) / 2.0, (Min.Z + Max.Z) / 2.0);
	}

	// Override equals operator
	bool Equals(const Box3& box) const
	{
		return Min == box.Min && Max == box.Max;
	}

	bool operator ==(const Box3& right)
	{
		return Equals(right);
	}

	bool operator !=(const Box3& right)
	{
		return !Equals(right);
	}

	// Split box into two along the given axis
	void split(IVertexUtilsBase::Axis axis, double position, Box3 boxes[2])
	{
		switch (axis) {
		case IVertexUtilsBase::X:
		{
			boxes[0] = Box3(Min, Vertex3(position, Max.Y, Max.Z));
			boxes[1] = Box3(Vertex3(position, Min.Y, Min.Z), Max);
		}
		break;
		case IVertexUtilsBase::Y:
		{
			boxes[0] = Box3(Min, Vertex3(Max.X, position, Max.Z));
			boxes[1] = Box3(Vertex3(Min.X, position, Min.Z), Max);
		}
		break;
		case IVertexUtilsBase::Z:
		{
			boxes[0] = Box3(Min, Vertex3(Max.X, Max.Y, position));
			boxes[1] = Box3(Vertex3(Min.X, Min.Y, position), Max);
		}
		break;
		}
	}
	void splitBox(IVertexUtilsBase::Axis axis, Box3 boxes[2])
	{
		switch (axis) {
		case IVertexUtilsBase::X:
		{
			boxes[0] = Box3(Min, Vertex3(Min.X + width() / 2, Max.Y, Max.Z));
			boxes[1] = Box3(Vertex3(Min.X + width() / 2, Min.Y, Min.Z), Max);
		}
		break;
		case IVertexUtilsBase::Y:
		{
			boxes[0] = Box3(Min, Vertex3(Max.X, Min.Y + height() / 2, Max.Z));
			boxes[1] = Box3(Vertex3(Min.X, Min.Y + height() / 2, Min.Z), Max);
		}
		break;
		case IVertexUtilsBase::Z:
		{
			boxes[0] = Box3(Min, Vertex3(Max.X, Max.Y, Min.Z + depth() / 2));
			boxes[1] = Box3(Vertex3(Min.X, Min.Y, Min.Z + depth() / 2), Max);
		}
		break;
		}
	}

};

class Face
{
public:
	int IndexA;
	int IndexB;
	int IndexC;

	Face(int indexA, int indexB, int indexC)
	{
		IndexA = indexA;
		IndexB = indexB;
		IndexC = indexC;
	}
	Face()
	{
		IndexA = 0;
		IndexB = 0;
		IndexC = 0;
	}
};

class FaceT : public Face
{
public:
	int TextureIndexA;
	int TextureIndexB;
	int TextureIndexC;

	int MaterialIndex;

	FaceT(int indexA, int indexB, int indexC, int textureIndexA, int textureIndexB,
		int textureIndexC, int materialIndex) : Face(indexA, indexB, indexC)
	{
		TextureIndexA = textureIndexA;
		TextureIndexB = textureIndexB;
		TextureIndexC = textureIndexC;

		MaterialIndex = materialIndex;
	}
};

class Material
{
public:
	std::string textureFile;
	std::string textureFileFullPath;
};
