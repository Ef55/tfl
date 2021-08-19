#include "AST.hpp"

ExprAST::ExprAST(internal::ExprASTImpl* ptr): _ast(ptr) {}
ExprAST ExprAST::number(double v) { return ExprAST(new internal::NumberExprAST(v)); }
ExprAST ExprAST::variable(std::string const& name) { return ExprAST(new internal::VariableExprAST(name)); }
ExprAST ExprAST::op(char op, ExprAST const& left, ExprAST const& right) { return ExprAST(new internal::BinaryExprAST(op, std::move(left), std::move(right))); }
ExprAST ExprAST::call(std::string const& callee, std::vector<ExprAST> const& args) { return ExprAST(new internal::CallExprAST(callee, std::move(args))); }


PrototypeAST::PrototypeAST(std::string const& name, std::vector<std::string> const& args): _name(name), _args(args) {}


FunctionAST::FunctionAST(PrototypeAST const& proto, ExprAST const& body): _proto(proto), _body(std::move(body)) {}


internal::NumberExprAST::NumberExprAST(double v): _val(v) {}
void internal::NumberExprAST::visit(ASTVisitor<void>& visitor) const {
    visitor.number(_val);
}

internal::VariableExprAST::VariableExprAST(std::string const& name): _name(name) {}
void internal::VariableExprAST::visit(ASTVisitor<void>& visitor) const {
    visitor.variable(_name);
}

internal::BinaryExprAST::BinaryExprAST(char op, ExprAST const& left, ExprAST const& right): _op(op), _left(std::move(left)), _right(std::move(right)) {}
void internal::BinaryExprAST::visit(ASTVisitor<void>& visitor) const {
    visitor.binary(_op, _left, _right);
}

internal::CallExprAST::CallExprAST(std::string const& callee, std::vector<ExprAST> const& args): _callee(callee), _args(std::move(args)) {}
void internal::CallExprAST::visit(ASTVisitor<void>& visitor) const {
    visitor.call(_callee, _args);
}
