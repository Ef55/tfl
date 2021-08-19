#include "Codegen.hpp"

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

CodeGenerator::CodeGenerator(): _context(), _module("Module", _context), _builder(_context), _named_values() {}

llvm::Value* CodeGenerator::number(double v) {
    return llvm::ConstantFP::get(context(), llvm::APFloat(v));
}

llvm::Value* CodeGenerator::variable(std::string const& name) {
    return _named_values.at(name);
}

llvm::Value* CodeGenerator::binary(char op, ExprAST const& left, ExprAST const& right) {
    auto l = codegen(left);
    auto r = codegen(right);
    switch(op) {
        case '+': return builder().CreateFAdd(l, r, "addtmp");
        case '-': return builder().CreateFSub(l, r, "subtmp");
        case '*': return builder().CreateFMul(l, r, "multmp");
        case '<': return builder().CreateUIToFP(
            builder().CreateFCmpULT(l, r, "cmptmp"),
            llvm::Type::getDoubleTy(context()),
            "booltmp"
        );
        default: throw std::invalid_argument("Unknown binary operator: " + std::to_string(op));
    }
}

llvm::Value* CodeGenerator::call(std::string const& callee, std::vector<ExprAST> const& args) {
    auto fun = _module.getFunction(callee);
    if(!fun) throw std::logic_error("Function `" + callee + "` not found.");
    if(fun->arg_size() != args.size()) throw std::logic_error("Argument count mismatch: " + std::to_string(fun->arg_size()) + "/" + std::to_string(args.size()));

    std::vector<llvm::Value*> cargs;
    for(auto arg : args) {
        cargs.push_back(codegen(arg));
    }

    return _builder.CreateCall(fun, cargs, "calltmp");
}

llvm::Function* CodeGenerator::prototype(std::string const& name, std::vector<std::string> const& args) {
    auto double_type = llvm::Type::getDoubleTy(_context);

    std::vector<llvm::Type*> types(args.size(), double_type);
    llvm::FunctionType* funt = llvm::FunctionType::get(double_type, types, false);
    llvm::Function* fun = llvm::Function::Create(funt, llvm::Function::ExternalLinkage, name, _module);

    std::vector<std::string>::size_type i = 0;
    for(auto& arg: fun->args()) {
        arg.setName(args[i]);
        ++i;
    }

    return fun;
}

llvm::Function* CodeGenerator::function(PrototypeAST const& proto, ExprAST const& body) {
    llvm::Function* fun = _module.getFunction(proto.name());

    if(!fun) {
        fun = codegen(proto);
    }

    if(!fun) throw std::logic_error("Function could not be generated");
    if(!fun->empty()) throw std::logic_error("Function cannot be redefined.");

    llvm::BasicBlock* block = llvm::BasicBlock::Create(_context, proto.name() + "_entry", fun);
    _builder.SetInsertPoint(block);

    _named_values.clear();
    for(auto& arg: fun->args()) {
        _named_values[static_cast<std::string>(arg.getName())] = static_cast<llvm::Value*>(&arg);
    }

    llvm::Value* cbody = codegen(body);
    if(!cbody) {
        fun->eraseFromParent();
        throw std::logic_error("Could not codegen function's body.");
    }

    _builder.CreateRet(cbody);
    llvm::verifyFunction(*fun);

    return fun;
}
