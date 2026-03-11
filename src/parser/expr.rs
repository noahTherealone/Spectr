
use crate::error::*;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Map,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ValueExpr {
    NilExpr,
    BoolExpr(bool),
    IntLiteralExpr(isize),
    FloatLiteralExpr(f64),
    StrLiteralExpr(String),
    BinaryExpr(BinaryOp, SBox<ValueExpr>, SBox<ValueExpr>),
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BasicType {
    Void,
    Bool,
    Int,
    Float,
    Str,
}

#[derive(Debug, Clone, PartialEq)]
pub enum TypeExpr {
    Basic(BasicType),
    List(SBox<TypeExpr>),
    Tuple(Vec<SBox<TypeExpr>>),
    Union(SBox<TypeExpr>, SBox<TypeExpr>),
    Inter(SBox<TypeExpr>, SBox<TypeExpr>),
    Arrow(SBox<TypeExpr>, SBox<TypeExpr>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum KindExpr {
    Value,
    Type,
    Kind,
    Functor(SBox<KindExpr>, SBox<KindExpr>),
    Named(Kind)
}

impl KindExpr {
    pub fn kind(&self) -> Kind {
        match self {
            KindExpr::Value => Kind::ValueKind,
            KindExpr::Type  => Kind::TypeKind,
            KindExpr::Kind  => Kind::KindKind,
            KindExpr::Functor(a, b) => Kind::Functor(Box::new(a.kind()), Box::new(b.kind())),
            KindExpr::Named(kind) => kind.clone()
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Kind {
    Value,
    Type,
    ValueKind,
    TypeKind,
    KindKind,
    Functor(Box<Kind>, Box<Kind>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct Identifier {
    pub name: String,
    pub kind: Kind,
    // later add reference to definition here
}

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Value(ValueExpr),
    Type(TypeExpr),
    Kind(KindExpr),
    Identifier(Identifier),
    Functor(SBox<Expr>, SBox<Expr>),
    FunctorCall(SBox<Expr>, SBox<Expr>),
}

impl Expr {
    pub fn kind(&self) -> Kind {
        match self {
            Expr::Value(_) => Kind::Value,
            Expr::Type(_)  => Kind::Type,
            Expr::Kind(kind) => kind.kind(),
            Expr::Identifier(id) => id.kind.clone(),
            Expr::Functor(arg, out) => Kind::Functor(
                Box::new(arg.kind()), Box::new(out.kind())
            ),
            Expr::FunctorCall(functor, arg) => {
                match functor.kind() {
                    Kind::Functor(input, out) => {
                        if arg.kind() == *input { *out }
                        else { panic!("argument of functor call doesn't match expected kind") }
                    },
                    _ => panic!("body of functor call should be a functor")
                }
            }
        }
    }
}