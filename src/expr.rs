
use crate::error::*;
use crate::kind::Kind;
use crate::lexer::TokenType;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Equals,
    Union,
    Inter,
    Map,
    Functor,
}

impl BinaryOp {
    pub fn from_token(tt: TokenType) -> Option<BinaryOp> {
        match tt {
            TokenType::Plus => Some(BinaryOp::Add),
            TokenType::Minus => Some(BinaryOp::Sub),
            TokenType::Star => Some(BinaryOp::Mul),
            TokenType::Slash => Some(BinaryOp::Div),
            TokenType::Modulus => Some(BinaryOp::Mod),
            TokenType::AND => Some(BinaryOp::And),
            TokenType::OR => Some(BinaryOp::Or),
            TokenType::XOR => Some(BinaryOp::Xor),
            TokenType::Ampersand => Some(BinaryOp::Inter),
            TokenType::Bar => Some(BinaryOp::Union),
            TokenType::RightArrow => Some(BinaryOp::Map),
            TokenType::DoubleRightArrow => Some(BinaryOp::Functor),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Identifier {
    pub name: String,
    // later add reference to definition here
}

#[derive(Debug, Clone, PartialEq)]
pub enum ExprShape {
    ValueKeyword, // may be: a kind, or a value of type kind
    TypeKeyword,  // may be: a kind, or a type, or a value of type kind, or a value of type type
    KindKeyword,  // may be: a kind, or a type, or a value of type kind, or a value of type type
    IdentifierExpr(Identifier),
    UnitExpr,
    NilLiteralExpr, // always means the unit value, whereas UnitExpr can be used in other contexts as well, e.g. empty function signatures
    BoolLiteralExpr(bool),
    IntLiteralExpr(usize),
    FloatLiteralExpr(f64),
    StrLiteralExpr(String),
    BinaryExpr(BinaryOp, Expr, Expr),
    ListExpr(Vec<Expr>),
    TupleExpr(Vec<Expr>),
    TypedTupleExpr(Vec<(Expr, Expr)>),
    FunctionCallExpr(Expr, Expr),
    HigherTupleExpr(Vec<Expr>),
    KindedTupleExpr(Vec<(Expr, Expr)>),
    FunctorCallExpr(Expr, Expr)
}

#[derive(Debug, Clone, PartialEq)]
pub struct Expr {
    pub shape: Box<ExprShape>,
    pub span: SourceRange,
    pub kind: Option<Kind>, // kind checker needs to fill this
}

#[derive(Debug, Clone, PartialEq)]
pub enum CompilerValue {
    Known, // make this a value container
    Runtime
}

impl Expr {
    pub fn new(shape: ExprShape, span: SourceRange) -> Expr {
        Expr {
            shape: Box::new(shape),
            span,
            kind: None
        }
    }
}