#pragma once

#include <memory>
#include <map>
#include <string>
#include <variant>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include "AST.hpp"

class CodeGenerator: public ASTVisitor<llvm::Value*> {
    llvm::LLVMContext _context;
    llvm::Module _module;
    llvm::IRBuilder<> _builder;
    std::map<std::string, llvm::Value*> _named_values;

    inline llvm::LLVMContext& context() { return _context; }
    inline llvm::Module& modul() { return _module; }
    inline llvm::IRBuilder<>& builder() { return _builder; }


    inline llvm::Value* codegen(ExprAST const& child){ return child.visit(*this); }
    inline llvm::Function* codegen(PrototypeAST const& proto){ return prototype(proto.name(), proto.args()); }
    inline llvm::Function* codegen(FunctionAST const& fun){ return function(fun.prototype(), fun.body()); }

public:
    CodeGenerator();


    llvm::Value* number(double v) override;
    llvm::Value* variable(std::string const& name) override;
    llvm::Value* binary(char op, ExprAST const& left, ExprAST const& right) override;
    llvm::Value* call(std::string const& callee, std::vector<ExprAST> const& args) override;
    llvm::Function* prototype(std::string const& name, std::vector<std::string> const& args);
    llvm::Function* function(PrototypeAST const& proto, ExprAST const& body);

    inline llvm::Module const& code() const { return _module; }

    inline llvm::Function* operator()(ExprAST const& child){ return codegen(FunctionAST(PrototypeAST("__tl_expr"), child)); }
    inline llvm::Function* operator()(PrototypeAST const& proto){ return codegen(proto); }
    inline llvm::Function* operator()(FunctionAST const& fun){ return codegen(fun); }

    inline llvm::Function* operator()(std::variant<ExprAST, PrototypeAST, FunctionAST> const& v){ 
        return std::visit([this](auto c){ return this->operator()(c); }, v); 
    }

};