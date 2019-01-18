#include <iostream>
#include <list>
#include <cmath>	// NAN
#include <cstdlib>	// strtod

class Expr
{
	friend std::ostream &operator<<(std::ostream &, const Expr &);
	friend std::istream &operator>>(std::istream &, Expr &);

public:
	Expr(std::istream &);
	double eval();

protected:
	typedef std::list<char>::iterator Lcit;
	typedef std::list<char>::const_iterator Lccit;
	typedef std::list<double>::iterator Ldit;
	typedef std::list<double>::const_iterator Ldcit;

	static void init(std::istream &);	// help with construction
	void reduce(Lcit&, Ldit&);			// help with eval()

	std::list<char> operators;
	std::list<double> operands;
};

std::ostream &operator<<(std::ostream &os, const Expr &expr)
{
	// suppose operands.size() == operators.size() + 1
	Expr::Lccit op = expr.operators.begin();
	Expr::Ldcit num = expr.operands.begin();
	while (op != expr.operators.end())
		os << *num++ << ' ' << *op++ << ' ';
	return os << *num;
}

std::istream &operator>>(std::istream &is, Expr &expr)
{
	try {
		expr.init(is);
	}
	catch (...) {
		is.setstate(is.failbit);
	}
}

Expr::Expr(std::istream &is)
{
	init(is);
}

double Expr::eval()
{
	// suppose operands.size() == operators.size() + 1
	if (operands.empty())
		return NAN;
	// suppose all operators are valid, i.e. +-*/
	// scan this->operators twice, first for * and /, then for + and -
	Lcit op = operators.begin();
	Ldit rhs = operands.begin();
	while (op != operators.end())
	{
		if (*op == '*' || *op == '/')
			reduce(op, rhs);
		else
			++op, ++rhs;
	}

	op = operators.begin(), rhs = operands.begin();
	while (op != operators.end())
		reduce(op, rhs);
	return *rhs;
}

void Expr::init(std::istream &is)
{
	std::string s;
	is >> s;
	std::strtod(s.c_str());
	// .............
	// https://en.cppreference.com/w/cpp/string/byte/strtof
	// https://en.cppreference.com/w/cpp/numeric/math/NAN
}

void Expr::reduce(Lcit &op, Ldit &rhs)
{
#ifndef NDEBUG
	std::cout << *this << '\n';
	std::cout << "rhs: " << *rhs
		<< " op: " << (op == operators.end() ? 'e' : *op) << '\n';
#endif
	Ldit lhs = rhs++;
	switch (*op)
	{
		case '+': (*lhs) += (*rhs); break;
		case '-': (*lhs) -= (*rhs); break;
		case '*': (*lhs) *= (*rhs); break;
		case '/': (*lhs) /= (*rhs); break;
	}
	rhs = --operands.erase(rhs);
	op = operators.erase(op);
}

using namespace std;
int main()
{
	Expr expr;
	cout << "value: " << expr.eval() << '\n';
	return 0;
}
