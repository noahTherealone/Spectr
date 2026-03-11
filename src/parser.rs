
use std::string::ParseError;

use crate::error::*;
use crate::lexer::*;

pub mod expr;
pub mod stmt;

use expr::Expr;
use expr::ValueExpr;
use stmt::Stmt;

pub struct Parser<'a> {
    file: &'a File<'a>,
    tokens: &'a [Token<'a>],
    pos: usize,
}

fn lbp(tt: TokenType) -> usize {
    match tt {
        TokenType::Plus => 100,
        TokenType::Minus => 100,
        TokenType::Star => 200,
        TokenType::Slash => 200,
        TokenType::Modulus => 80,
        TokenType::Ampersand => 70,
        TokenType::Bar => 60,
        TokenType::RightArrow => 20,
        _ => 0,
    }
}

fn rbp(tt: TokenType) -> usize {
    match tt {
        TokenType::RightArrow => lbp(tt) + 1,
        _ => lbp(tt),
    }
}

fn wrap_first<A, B>((a, b): (A, B)) -> (Option<A>, B) {
    (Some(a), b)
}

impl<'a> Parser<'a> {
    fn new(tokens: &'a [Token<'a>], file: &'a File<'a>) -> Parser<'a> {
        Parser { file, tokens, pos: 0 }
    }

    fn peek(&self) -> Option<&Token<'a>> {
        self.tokens.get(self.pos)
    }

    fn peek_type(&self) -> Option<TokenType> {
        self.tokens.get(self.pos).map(|t| t.token_type)
    }

    fn next(&mut self) -> Option<&Token<'a>> {
        let tok = self.tokens.get(self.pos)?;
        self.pos += 1;
        Some(tok)
    }

    fn parse_expr(&mut self) -> (SBox<Expr>, Vec<SpectrError>) {
        (SBox::new(Expr::Value(ValueExpr::NilExpr), None), Vec::new()) // just so it compiles, here we'll add a Pratt parser later
    }

    fn parse_if_stmt(&mut self) -> (SBox<Stmt>, Vec<SpectrError>) {
        (SBox::new(
            Stmt::IfElseStmt(
                Expr::Value(ValueExpr::BoolExpr(true)),
                Vec::new(), Vec::new()
            ),
            None
        ), Vec::new())
    }

    fn parse_while_stmt(&mut self) -> (SBox<Stmt>, Vec<SpectrError>) {
        (SBox::new(Stmt::WhileStmt(Expr::Value(ValueExpr::BoolExpr(true)), Vec::new()), None), Vec::new())
    }

    fn parse_stmt(&mut self) -> (Option<SBox<Stmt>>, Vec<SpectrError>) {
        let tt = self.peek().map(|t| t.token_type);
        let file = self.file;
        match tt {
            Some(TokenType::LnBreak) => {
                self.next();
                self.parse_stmt()
            }
            Some(TokenType::IF) => wrap_first(self.parse_if_stmt()),
            Some(TokenType::WHILE) => wrap_first(self.parse_while_stmt()),
            Some(_) => {
                let (wrapped, span, mut errs) = {
                    let (expr, errs) = self.parse_expr();
                    let SBox { span, node } = expr;
                    (Some(SBox::new(Stmt::ExprStmt(*node), span.clone())), span, errs)
                };

                let next_tt = self.peek_type();
                match next_tt {
                    Some(TokenType::LnBreak) => (wrapped, errs),
                    Some(_) => {
                        errs.push(SpectrError {
                            error_type: SpectrErrorType::ParserError,
                            src: span.unwrap(),
                            msg: "unexpected token after statement".to_string()
                        });
                        (wrapped, errs)
                    },
                    None => (wrapped, errs),
                }
            }
            None => (None, Vec::new())
        }
    }

    pub fn parse(tokens: &'a [Token<'a>], file: &'a File<'a>) -> Vec<Stmt> {
        let mut parser = Parser::new(tokens, file);
        let mut stmts: Vec<Stmt> = Vec::new();

        loop {
            let (opt, errs) = parser.parse_stmt();

            for err in errs {
                eprintln!("{}", err.display(parser.file));
            }

            if let Some(stmt) = opt {
                stmts.push(*stmt.node);
            } else {
                return stmts;
            }
        }
    }
}