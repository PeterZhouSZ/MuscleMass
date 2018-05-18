#include <iostream>
#include <math.h> // atan
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Rigid.h"
#include "Joint.h"
#include "Shape.h"
#include "Program.h"
#include "MatrixStack.h"

using namespace std;
using namespace Eigen;

Rigid::Rigid() {

}

Rigid::Rigid(const shared_ptr<Shape> s, Matrix3d _R, Vector3d _p, Vector3d _dimension, double _r, double _m, bool _isReduced) :
	r(_r),
	m(_m),
	i(-1),
	box(s),
	dimension(_dimension),
	grav(0.0, -9.8, 0.0),
	isReduced(_isReduced)
{
	this->twist.setZero();
	this->force.setZero();

	E_W_0.setZero();
	E_W_0(3, 3) = 1.0;
	setP(_p);
	setR(_R);

	mass_mat.setZero();
	double d0 = dimension(0);
	double d1 = dimension(1);
	double d2 = dimension(2);
	this->mass_mat << m / 12.0 * (d1 * d1 + d2 * d2), 0, 0, 0, 0, 0,
					  0, m / 12.0 * (d0 * d0 + d2 * d2), 0, 0, 0, 0,
					  0, 0, m / 12.0 * (d1 * d1 + d0 * d0), 0, 0, 0,
					  0, 0, 0, m, 0, 0,
					  0, 0, 0, 0, m, 0,
					  0, 0, 0, 0, 0, m;
	
	this->joint = make_shared<Joint>();

}

void Rigid::tare()
{
	twist.setZero();
}

void Rigid::reset()
{
	twist.setZero();
	E_W_0 = E_W_0_0;
}

void Rigid::step(double h) {
	computeForces();
	isReduced = true;
	// Position Update
	if (isReduced) {
		// Use reduced positions
		if (i != 0) {
			Matrix4d E_J_C = joint->getE_C_J().inverse();

			double theta = joint->getTheta();
			//cout << "theta" << theta << endl;

			Matrix4d R;
			R.setIdentity();
			R.block<2, 2>(0, 0) << cos(theta), -sin(theta),
				sin(theta), cos(theta);

			Matrix4d E_P_J = joint->getE_P_J();
			Matrix4d E_W_P = parent->getE();
			Matrix4d E_W_C = E_W_P * E_P_J * R * E_J_C;
			E_W_0 = E_W_C;
		}
	}
	else {
		// Use maximal coordinate
		if (i != 0) {
			E_W_0 = integrate(E_W_0, twist, h);
		}
		
	}

	// Joint Update
	if (i != 0) {
		Matrix4d E_C_J = getE().inverse() * parent->getE() * joint->getE_P_J();
		joint->setE_C_J(E_C_J);
	}
	
}

void Rigid::draw(shared_ptr<MatrixStack> MV, const shared_ptr<Program> prog) const
{
	if (box) {
		MV->pushMatrix();
		Vector3d x = getP();
		MV->translate(x(0), x(1), x(2));
		
		// Decompose R into 3 Euler angles
		Matrix3d R = getR();
		double theta_x = atan2(R(2, 1), R(2, 2));
		double theta_y = atan2(-R(2, 0), sqrt(pow(R(2, 1), 2) + pow(R(2, 2), 2)));
		double theta_z = atan2(R(1, 0), R(0, 0));
		MV->rotate(theta_x, 1.0f, 0.0f, 0.0f);
		MV->rotate(theta_y, 0.0f, 1.0f, 0.0f);
		MV->rotate(theta_z, 0.0f, 0.0f, 1.0f);

		MV->scale(r);
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		box->draw(prog);
		MV->popMatrix();
	}
}

Matrix4d Rigid::inverse(const Matrix4d &E)
{
	Matrix4d Einv = Matrix4d::Identity();
	Matrix3d R = E.block<3, 3>(0, 0);
	Vector3d p = E.block<3, 1>(0, 3);
	Matrix3d Rt = R.transpose();
	Einv.block<3, 3>(0, 0) = Rt;
	Einv.block<3, 1>(0, 3) = -Rt * p;
	return Einv;
}

Matrix3x6d Rigid::gamma(const Eigen::Vector3d &r)
{
	Matrix3x6d G = Matrix3x6d::Zero();
	G.block<3, 3>(0, 0) = bracket3(r).transpose();
	G.block<3, 3>(0, 3) = Matrix3d::Identity();
	return G;
}

Matrix6d Rigid::adjoint(const Matrix4d &E)
{
	Matrix6d Ad = Matrix6d::Zero();
	Matrix3d R = E.block<3, 3>(0, 0);
	Vector3d p = E.block<3, 1>(0, 3);
	Ad.block(0, 0, 3, 3) = R;
	Ad.block(3, 0, 3, 3) = bracket3(p) * R;
	Ad.block(3, 3, 3, 3) = R;
	return Ad;
}

Matrix3d Rigid::bracket3(const Vector3d &a)
{
	Matrix3d A = Matrix3d::Zero();
	A(0, 1) = -a(2);
	A(0, 2) = a(1);
	A(1, 0) = a(2);
	A(1, 2) = -a(0);
	A(2, 0) = -a(1);
	A(2, 1) = a(0);
	return A;
}

Matrix4d Rigid::bracket6(const Vector6d &a)
{
	Matrix4d A = Matrix4d::Zero();
	A.block<3, 3>(0, 0) = bracket3(a.segment<3>(0));
	A.block<3, 1>(0, 3) = a.segment<3>(3);
	return A;
}

Vector3d Rigid::unbracket3(const Matrix3d &A)
{
	Vector3d a;
	a(0) = A(2, 1);
	a(1) = A(0, 2);
	a(2) = A(1, 0);
	return a;
}

Vector6d Rigid::unbracket6(const Matrix4d &A)
{
	Vector6d a;
	a.segment<3>(0) = unbracket3(A.block<3, 3>(0, 0));
	a(3) = A(0, 3);
	a(4) = A(1, 3);
	a(5) = A(2, 3);
	return a;
}

Matrix4d Rigid::integrate(const Matrix4d &E0, const VectorXd &phi, double h)
{
	Matrix3d I = Matrix3d::Identity();
	Vector3d w = phi.segment<3>(0);
	Vector3d v = phi.segment<3>(3);
	Matrix4d phib = Matrix4d::Identity();
	phib.block<3, 1>(0, 3) = h*v;
	double wlen = w.norm();
	if (wlen > 1e-10) {
		w /= wlen;
		v /= wlen;
		// Rodrigues formula %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		double wX = w(0);
		double wY = w(1);
		double wZ = w(2);
		double c = cos(wlen * h);
		double s = sin(wlen * h);
		double c1 = 1.0 - c;
		Matrix3d R;
		R << c + wX * wX * c1, -wZ * s + wX * wY * c1, wY * s + wX * wZ * c1,
			wZ * s + wX * wY * c1, c + wY * wY * c1, -wX * s + wY * wZ * c1,
			-wY * s + wX * wZ * c1, wX * s + wY * wZ * c1, c + wZ * wZ * c1;
		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		Matrix3d A = I - R;
		Vector3d cc = w.cross(v);
		Vector3d d = A * cc;
		double wv = w.dot(v);
		Vector3d p = (wv * wlen * h) * w + d;
		phib.block<3, 3>(0, 0) = R;
		phib.block<3, 1>(0, 3) = p;
		//cout << phib << endl;
	}
	return E0 * phib;
}

void Rigid::computeForces() {
	Matrix6d twist_bracket;
	twist_bracket.setZero();
	twist_bracket.block<3, 3>(0, 0) = bracket3(twist.segment<3>(0));
	twist_bracket.block<3, 3>(3, 3) = bracket3(twist.segment<3>(0));

	Vector6d coriolis_forces = twist_bracket.transpose() * mass_mat * twist;
	Vector6d body_forces;
	body_forces.setZero();
	body_forces.segment<3>(3) = m * getR().transpose() * grav;
	force = coriolis_forces + body_forces;
}

// get
Eigen::Vector3d Rigid::getP() const {
	return E_W_0.block<3, 1>(0, 3);
}

Eigen::Matrix3d Rigid::getR() const {
	return E_W_0.block<3, 3>(0, 0);
}

Matrix6d Rigid::getMassMatrix() const {
	return mass_mat;
}

Vector6d Rigid::getTwist() const {
	return twist;
}

Vector6d Rigid::getForce() const {
	return force;
}

shared_ptr<Joint> Rigid::getJoint() const {
	return joint;
}

Matrix4d Rigid::getE() const {
	return E_W_0;
}

shared_ptr<Rigid> Rigid::getParent() const {
	return parent;
}

int Rigid::getIndex() const {
	return i;
}

// set
void Rigid::setP(Eigen::Vector3d p) {
	E_W_0.block<3, 1>(0, 3) = p;
}

void Rigid::setR(Eigen::Matrix3d R) {
	E_W_0.block<3, 3>(0, 0) = R;
}

void Rigid::setTwist(Vector6d _twist) {
	twist = _twist;
}

void Rigid::setForce(Vector6d _force) {
	force = _force;
}

void Rigid::setIndex(int _i) {
	i = _i;
}

void Rigid::setParent(shared_ptr<Rigid> _parent) {
	parent = _parent;
}

void Rigid::setJointAngle(double _theta) {
	joint->setTheta(_theta);
}

void Rigid::addChild(shared_ptr<Rigid> _child) {
	children.push_back(_child);
}

Rigid::~Rigid()
{
}