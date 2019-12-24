#pragma once

#include <string>
#include <vector>

#include "expr.h"

namespace torch {
namespace jit {
namespace fuser {

enum IRNodeType {
  kAdd,
  kSub,
  kMul,
  kDiv,
};

class Cast : public ExprNode<Cast> {
 public:
  const Expr& src_value() const { return src_value_; }
  static Expr make(Dtype dtype, const Expr& src_value) { return Expr(new Cast(dtype, src_value)); }

 private:
  Cast(Dtype dtype, const Expr& src_value) : ExprNodeBase(dtype), src_value_(src_value) {}
  Expr src_value_;
};

template <typename T>
Expr cast(const Expr& src_value) {
  return Cast::make(Dtype(ToDtype<T>(), src_value.dtype().lanes()), src_value);
}

// Represent the expression node for binary operators.
// A CRTP pattern to share common code among the operators.
template <typename Op>
class BinaryOpNode : public ExprNode<Op> {
 public:
  const Expr& lhs() const { return this->lhs_; }
  const Expr& rhs() const { return this->rhs_; }
  IRNodeType expr_type() const { return expr_type_; }

  static Expr make(const Expr& lhs, const Expr& rhs) { return Expr(new Op(lhs, rhs)); }

 protected:
  BinaryOpNode(const Expr& lhs_v, const Expr& rhs_v, IRNodeType expr_type)
      : ExprNode<Op>(BinaryOpDtype(lhs_v.dtype(), rhs_v.dtype())),
        lhs_(CastIfNeeded(lhs_v, ExprNode<Op>::dtype())),
        rhs_(CastIfNeeded(rhs_v, ExprNode<Op>::dtype())),
        expr_type_(expr_type) {}

 private:
  static Expr CastIfNeeded(const Expr& expr, Dtype dst_dtype) {
    if (expr.dtype() == dst_dtype) {
      return expr;
    }
    return Cast::make(dst_dtype, expr);
  }

  Expr lhs_;
  Expr rhs_;
  IRNodeType expr_type_;
};

class Add : public BinaryOpNode<Add> {
 private:
  Add(const Expr& lhs, const Expr& rhs) : BinaryOpNode(lhs, rhs, IRNodeType::kAdd) {}
  friend class BinaryOpNode<Add>;
};

class Sub : public BinaryOpNode<Sub> {
 private:
  Sub(const Expr& lhs, const Expr& rhs) : BinaryOpNode(lhs, rhs, IRNodeType::kSub) {}
  friend class BinaryOpNode<Sub>;
};

class Mul : public BinaryOpNode<Mul> {
 private:
  Mul(const Expr& lhs, const Expr& rhs) : BinaryOpNode(lhs, rhs, IRNodeType::kMul) {}
  friend class BinaryOpNode<Mul>;
};

class Div : public BinaryOpNode<Div> {
 private:
  Div(const Expr& lhs, const Expr& rhs) : BinaryOpNode(lhs, rhs, IRNodeType::kDiv) {}
  friend class BinaryOpNode<Div>;
};

// Encode an integer immediate value.
class IntImm : public ExprNode<IntImm> {
 public:
  int value() const { return value_; }
  static Expr make(int value) { return Expr(new IntImm(value)); }

 private:
  IntImm(int value) : ExprNodeBase(kInt32), value_(value) {}
  int value_;
};

// Encode an fp32 immediate value.
class FloatImm : public ExprNode<FloatImm> {
 public:
  float value() const { return value_; }
  static Expr make(float value) { return Expr(new FloatImm(value)); }

 private:
  FloatImm(float value) : ExprNodeBase(kFloat32), value_(value) {}
  float value_;
};

// The underlying representation node to a Variable.
// Currently, each Variable object represents a unique variable, even though the names
// might be the same. We should consider add a unique_name as well.
class Variable : public ExprNode<Variable> {
 public:
  static Expr make(const std::string& name_hint, Dtype dtype) {
    return Expr(new Variable(name_hint, dtype));
  }
  static Expr make(Dtype dtype) { return Expr(new Variable("", dtype)); }

  // TODO: unique_name
  const std::string& name_hint() const {
    return name_hint_;
  }

 private:
  Variable(const std::string& name_hint, Dtype dtype)
      : ExprNodeBase(dtype), name_hint_(name_hint) {}
  std::string name_hint_;
};

class Block : public ExprNode<Block> {
 public:
  static Expr make(const std::vector<Expr>& exprs) { return Expr(new Block(exprs)); }
  int nexprs() const { return exprs_.size(); }
  const Expr& expr(int index) const { return exprs_[index]; }

 private:
  explicit Block(const std::vector<Expr>& exprs) : ExprNodeBase(kNull), exprs_(exprs) {}
  std::vector<Expr> exprs_;
};

class For : public ExprNode<For> {
 public:
  const Expr& var() const { return var_; }
  const Expr& start() const { return start_; }
  const Expr& stop() const { return stop_; }
  const Expr& body() const { return body_; }
  static Expr make(const Expr& var, const Expr& start, const Expr& stop, const Expr& body) {
    return Expr(new For(var, start, stop, body));
  }

 private:
  For(const Expr& var, const Expr& start, const Expr& stop, const Expr& body)
      : ExprNodeBase(kNull), var_(var), start_(start), stop_(stop), body_(body) {
        //TODO! make sure var is of type Variable
      }
  Expr var_;
  Expr start_;
  Expr stop_;
  Expr body_;
};

//Dummy expr for testing
class EmptyExpr : public  ExprNode<EmptyExpr> {
  public:
    static Expr make (){ return Expr(new EmptyExpr()); }
  private:
    EmptyExpr():ExprNodeBase(kNull){}
};

} // namespace fuser
} // namespace jit
} // namespace torch