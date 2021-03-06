#pragma once
#ifndef MUSCLEMASS_SRC_MLOBJECT_H_
#define MUSCLEMASS_SRC_MLOBJECT_H_

#include <string>
#include <vector>

#include "MLCommon.h"

class MLError;
class MLWorld;
class MatrixStack;
class Program;

class MLObject {
public:
	explicit MLObject();
	virtual ~MLObject();

	virtual void init() {}
	virtual void draw(std::shared_ptr<MatrixStack> MV, const std::shared_ptr<Program> prog) const {}

	virtual MLError load(const nlohmann::json &elem, std::shared_ptr<MLWorld> world, const std::string &folder);
	virtual void save(std::ofstream &ofs) const;

	std::string getName() const { return m_name; }
	int getUID() const { return m_uid; }

protected:
	std::string m_name;
	int m_uid;

public:
	static int UIDMAX;

	int genNextUID();
};

#endif // MUSCLEMASS_SRC_MLOBJECT_H_