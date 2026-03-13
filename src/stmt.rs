
use crate::error::*;
use crate::expr::Expr;

#[derive(Debug, Clone, PartialEq)]
pub enum StmtShape {
    ExprStmt(Expr),
    IfElseStmt(Expr, Vec<Stmt>, Vec<Stmt>),
    WhileStmt(Expr, Vec<Stmt>),
    ExplicitDecl(Expr, Expr, Expr), // <name> : <type> = <value>
    UnspecifiedDecl(Expr, Expr), // <name> : <type>
    InferredDecl(Expr, Expr), // <name> := <value>
    ExplicitDefn(Expr, Expr, Expr), // <name> :: <kind> = <object>
    UnspecifiedDefn(Expr, Expr), // <name> :: <kind>
    InferredDefn(Expr, Expr), // <name> ::= <object>
}

#[derive(Debug, Clone, PartialEq)]
pub struct Stmt {
    pub shape: StmtShape,
    pub span: SourceRange,
}

impl Stmt {
    pub fn new(shape: StmtShape, span: SourceRange) -> Stmt {
        Stmt { shape, span }
    }
}