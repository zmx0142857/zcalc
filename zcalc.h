#ifndef ZCALC_H
#define ZCALC_H

#include <iostream>
#include <sstream>
#include <cctype>
#include <cmath>
#include <stack>

enum TokenType {
	END,				// end of file
	NUM,				// int and double
	ADD, SUB, MUL, DIV,	// + - * /
	LPAREN, RPAREN		// ()
};

const char *token_type[] = {
	"end", "num", "+", "-", "*", "/", "(", ")"
};

struct Token {
	TokenType type;
	Token(TokenType t): type(t) {}
	virtual ~Token();
	virtual std::string to_str() const;
	virtual double val() const;
};

struct Num: public Token {
	double value;
	Num(double v): Token(NUM), value(v) {}
	virtual std::string to_str() const;
	virtual double val() const;
};

Token::~Token() {}

std::string Token::to_str() const
{
	return token_type[type];
}

std::string Num::to_str() const
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

double Token::val() const
{
	return 0;
}

double Num::val() const
{
	return value;
}

struct LexError {
	std::string detail;
	LexError(const std::string &s): detail(s) {}
};

class Lexer {
	friend class Calc;

	std::istream &is;
	char ch;
	size_t index;
	Token *token;

	void get_char() {
		ch = is.get();
		++index;
	}

	unsigned read_int(int &val) {
		unsigned cnt = 0;
		while (isdigit(ch)) {
			++cnt;
			val = 10 * val + (ch - '0');
			get_char();
		}
		return cnt;
	}

	Token *match() {
		if (ch == '\n') {
			ch = ' ';
			return new Token(END);
		}

		// ignore space
		while (isspace(ch))
			get_char();

		// matches number
		if (isdigit(ch) || ch == '.') {
			int val1 = 0, val2 = 0, val3 = 0;
			unsigned val2_cnt = 0;
			read_int(val1);
			if (ch == '.') {
				get_char();
				if (!isdigit(ch))
					throw LexError("missing value after decimal dot");
				val2_cnt = read_int(val2);
				if (val2 == 0)
					val2_cnt = 0;
			}
			if (ch == 'e' || ch == 'E') {
				get_char();
				bool neg = false;
				if (ch == '+')
					get_char();
				else if (ch == '-') {
					neg = true;
					get_char();
				}
				if (!isdigit(ch))
					throw LexError("missing value after scientific"
							"notation E");
				read_int(val3);
				if (neg)
					val3 = -val3;
			}

			return new Num((val1 + val2 * pow(0.1, val2_cnt))
					* pow(10, val3));
		}

		// match operators
		else {
			switch (ch) {
				case '+': get_char(); return new Token(ADD);
				case '-': get_char(); return new Token(SUB);
				case '*': get_char(); return new Token(MUL);
				case '/': get_char(); return new Token(DIV);
				case '(': get_char(); return new Token(LPAREN);
				case ')': get_char(); return new Token(RPAREN);
				case EOF: case 0: return new Token(END);
				default:  std::ostringstream oss;
						  oss << "invalid token '" << ch << "' (ascii "
							  << int(ch) << " )";
						  throw LexError(oss.str());
			}
		}
	}

	void set_token(Token *t) {
		if (token)
			delete token;
		token = t;
	}

public:
	Lexer(std::istream &i): is(i), ch(is.get()), index(), token(NULL) {}

	~Lexer() {
		set_token(NULL);
	}

	Token *get_token() {
		if (is) {
			try {
				set_token(match());
			} catch (LexError e) {
				std::cout << std::string(index, ' ')  << '^' << '\n'
					<< "lexical error: " << e.detail << std::endl;
				return abort(false);
			}
		} else {
			set_token(new Token(END));
		}
#ifdef DEBUG
		std::cout << "token: " << token->to_str() << std::endl;
#endif
		return token;
	}

	Token *abort(bool goback=true) {
		while (ch != '\n' && ch != EOF)
			ch = is.get();
		set_token(new Token(END));
		if (goback)
			--index;
#ifdef DEBUG
		if (index == size_t(-1))
			std::cout << "index == -1" << std::endl;
#endif
		return token;
	}
	
#ifdef DEBUG
	static void test() {
		using namespace std;
		Lexer lex(cin);
		Token *token;
		do {
			token = lex.get_token();
		} while (token->type != END);
	}
#endif
};

struct SyntaxError {
	std::string detail;
	SyntaxError(const std::string &s): detail(s) {}
};


/* expr		::= firstitem exprtail
 * exprtail	::= adds item exprtail | <empty>
 * adds		::= + | -
 * firstitem::= item | <empty>
 * item		::= factor itemtail
 * itemtail	::= muls factor itemtail | <empty>
 * muls		::= * | /
 * factor	::= ( expr ) | NUM
 */

class Calc {
	Lexer lexer;
	Token *token;
	std::stack<double> nums;

	void match_expr() {
		match_firstitem();
		match_exprtail();
	}

	void match_exprtail() {
		while (token->type == ADD || token->type == SUB) {
			TokenType op = token->type;
			token = lexer.get_token();
			if (token->type == END)
				throw SyntaxError("missing operand");
			if (!match_item()) {
				std::string tmp = token->to_str();
				token = lexer.abort();
				throw SyntaxError(std::string("redudant operator '")
						+ tmp + "'");
			}
#ifdef DEBUG
			if (nums.size() <= 1)
				std::cout << "match_exprtail: stack size == " <<
					nums.size() << std::endl;
#endif
			double tmp = nums.top();
			nums.pop();
			if (op == ADD)
				nums.top() += tmp;
			else
				nums.top() -= tmp;
		}
		if (token->type != RPAREN && token->type != END)
			throw SyntaxError("missing operator");
	}

	void match_firstitem() {
		if (!match_item()) {
			if (token->type == ADD || token->type == SUB) {
				nums.push(0);
			} else if (token->type == END) {
				return;
			} else { // token->type == RPAREN
				token = lexer.abort();
				throw SyntaxError("missing expr");
			}
		}
	}

	bool match_item() {
		if (!match_factor())
			return false;
		match_itemtail();
		return true;
	}

	void match_itemtail() {
		while (token->type == MUL || token->type == DIV) {
			TokenType op = token->type;
			token = lexer.get_token();
			if (token->type == END)
				throw SyntaxError("missing operand");
			if (!match_factor()) {
				std::string tmp = token->to_str();
				token = lexer.abort();
				throw SyntaxError(std::string("redudant operator '")
					+ tmp + "'");
			}
#ifdef DEBUG
			if (nums.size() <= 1)
				std::cout << "match_itemtail: stack size == " <<
					nums.size() << std::endl;
#endif
			double tmp = nums.top();
			nums.pop();
			if (op == MUL)
				nums.top() *= tmp;
			else
				nums.top() /= tmp;
		}
		if (token->
				type != ADD && token->type != SUB
				&& token->type != RPAREN && token->type != END)
			throw SyntaxError("missing operator");
	}

	bool match_factor() {
		if (token->type == LPAREN) {
			token = lexer.get_token();
			if (token->type == END)
				throw SyntaxError("missing ')'");
			match_expr();
			if (token->type == END)
				throw SyntaxError("missing ')'");
			if (token->type == RPAREN) {
				token = lexer.get_token();
				return true;
			} else {
				token = lexer.get_token();
				throw SyntaxError("missing ')'");
			}
		} else if (token->type == NUM) {
			nums.push(token->val());
			token = lexer.get_token();
			return true;
		}
		return false;
	}

public:
	Calc(std::istream &is): lexer(is), token(lexer.get_token()) { }

	void eval() {
		// empty the stack
		while (!nums.empty())
			nums.pop();

		if (token->type == END)
			token = lexer.get_token();

		try {
			match_expr();
			// empty stack means empty expression
			if (!nums.empty())
				 std::cout << nums.top() << std::endl;
		//} catch (LexError le) {
			//token = lexer.abort();
		} catch (SyntaxError se) {
			std::cout << std::string(lexer.index, ' ')  << '^' << '\n'
				<< "syntax error: " << se.detail << std::endl;
		}
		lexer.index = -1;
	}

	void run() {
		while (lexer.is) {
			eval();
		}
	}
};

#endif // ZCALC_H
