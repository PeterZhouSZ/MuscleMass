#pragma once

#ifndef MUSCLEMASS_SRC_RIGID_H_
#define MUSCLEMASS_SRC_RIGID_H_

#include <vector>
#include <memory>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

class Shape;
class Program;
class MatrixStack;
class Joint;

typedef Eigen::Matrix<double, 6, 1> Vector6d;
typedef Eigen::Matrix<double, 6, 6> Matrix6d;
typedef Eigen::Matrix<double, 3, 6> Matrix3x6d;
typedef Eigen::Matrix<double, 6, 3> Matrix6x3d;
typedef Eigen::Matrix<double, 5, 6> Matrix5x6d;

class Rigid
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
		Rigid();
	Rigid(const std::shared_ptr<Shape> shape, Eigen::Matrix3d _R, Eigen::Vector3d _p, Eigen::Vector3d _dimension, double _r, double _m, bool _isReduced);
	virtual ~Rigid();
	void tare();
	void reset();
	void step(double h);
	void draw(std::shared_ptr<MatrixStack> MV, const std::shared_ptr<Program> p) const;
	void computeForces();

	// set
	void setIndex(int _i);
	void setP(Eigen::Vector3d p);
	void setR(Eigen::Matrix3d R);
	void setTwist(Vector6d _twist);
	void setForce(Vector6d _force);
	void setParent(std::shared_ptr<Rigid> _parent);
	void addChild(std::shared_ptr<Rigid> _child);
	void setJointAngle(double _theta);

	// get
	Eigen::Vector3d getP() const;
	Eigen::Matrix3d getR() const;
	Matrix6d getMassMatrix() const;
	Vector6d getTwist() const;
	Vector6d getForce() const;
	std::shared_ptr<Joint> getJoint() const;
	Eigen::Matrix4d getE() const;
	std::shared_ptr<Rigid> getParent() const;
	int getIndex() const;

	static Eigen::Matrix4d inverse(const Eigen::Matrix4d &E);
	static Matrix3x6d gamma(const Eigen::Vector3d &r);
	static Matrix6d adjoint(const Eigen::Matrix4d &E);
	static Eigen::Matrix3d bracket3(const Eigen::Vector3d &a);
	static Eigen::Matrix4d bracket6(const Vector6d &a);
	static Eigen::Vector3d unbracket3(const Eigen::Matrix3d &A);
	static Vector6d unbracket6(const Eigen::Matrix4d &A);
	static Eigen::Matrix4d integrate(const Eigen::Matrix4d &E0, const Eigen::VectorXd &phi, double h);

	double r; // radius
	double m; // mass
	bool isReduced; 
	
	Eigen::Vector3d dimension;
	Eigen::Vector3d grav;	

private:
	const std::shared_ptr<Shape> box;
	std::shared_ptr<Rigid> parent;
	std::vector< std::shared_ptr<Rigid> > children;
	std::shared_ptr<Joint> joint;
	Eigen::Matrix4d E_W_0; // Where current transform is wrt world
	Eigen::Matrix4d E_W_0_0; // Where current transform is wrt world at the start
	Matrix6d mass_mat;
	Vector6d twist;
	Vector6d force;
	int i;  // index
};
#endif // MUSCLEMASS_SRC_RIGID_H_