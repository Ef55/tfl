#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

class ExprAST;
class PrototypeAST;
class FunctionAST;

template<typename R>
class ASTVisitor {
public:
    virtual R number(double v) = 0;
    virtual R variable(std::string const& name) = 0;
    virtual R binary(char op, ExprAST const& left, ExprAST const& right) = 0;
    virtual R call(std::string const& callee, std::vector<ExprAST> const& args) = 0;
};

namespace internal { 
    class ExprASTImpl {
    public:
        virtual ~ExprASTImpl() = default;
        virtual void visit(ASTVisitor<void>& visitor) const = 0;
    };
}

class ExprAST final {
    std::shared_ptr<internal::ExprASTImpl> _ast;
    ExprAST(internal::ExprASTImpl* ptr);

    template<typename R>
    struct VoidedASTVisitor: ASTVisitor<void> {
        ASTVisitor<R>* underlying;
        std::optional<R> result;

        VoidedASTVisitor(ASTVisitor<R>& under): underlying(&under), result(std::nullopt) {}

        void number(double v) override {
            result = std::optional{underlying->number(v)};
        }
        void variable(std::string const& name) override {
            result = std::optional{underlying->variable(name)};
        }
        void binary(char op, ExprAST const& left, ExprAST const& right) override {
            result = std::optional{underlying->binary(op, left, right)};
        }
        void call(std::string const& callee, std::vector<ExprAST> const& args) override {
            result = std::optional{underlying->call(callee, args)};
        }
    };
public:
    static ExprAST number(double v);
    static ExprAST variable(std::string const& name);
    static ExprAST op(char op, ExprAST const& left, ExprAST const& right);
    static ExprAST call(std::string const& callee, std::vector<ExprAST> const& args);

    template<typename R>
    R visit(ASTVisitor<R>& visitor) const {
        VoidedASTVisitor<R> vvis{visitor};
        _ast->visit(vvis);
        return vvis.result.value();
    }
};

class PrototypeAST final {
    std::string _name;
    std::vector<std::string> _args;
public:
    PrototypeAST(std::string const& name, std::vector<std::string> const& args = {});
    inline std::string const& name() const { return _name; };
    inline std::vector<std::string> const& args() const { return _args; };
};

class FunctionAST final {
    PrototypeAST _proto;
    ExprAST _body;
public:
    FunctionAST(PrototypeAST const& proto, ExprAST const& body);

    inline PrototypeAST const& prototype() const { return _proto; };
    inline ExprAST const& body() const { return _body; };
};

namespace internal {

    class NumberExprAST: public ExprASTImpl {
        double _val;
    public:
        NumberExprAST(double v);
        virtual void visit(ASTVisitor<void>& visitor) const override;
    };

    class VariableExprAST: public ExprASTImpl {
        std::string _name;
    public:
        VariableExprAST(std::string const& name);
        virtual void visit(ASTVisitor<void>& visitor) const override;
    };

    class BinaryExprAST: public ExprASTImpl {
        char _op;
        ExprAST _left;
        ExprAST _right;
    public:
        BinaryExprAST(char op, ExprAST const& left, ExprAST const& right);
        virtual void visit(ASTVisitor<void>& visitor) const override;
    };

    class CallExprAST: public ExprASTImpl {
        std::string _callee;
        std::vector<ExprAST> _args;
    public:
        CallExprAST(std::string const& callee, std::vector<ExprAST> const& args);
        virtual void visit(ASTVisitor<void>& visitor) const override;
    };
}
