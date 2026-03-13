
use std::fs::OpenOptions;

use super::Parser;
use crate::error::*;
use crate::lexer::{Token, TokenType};
use crate::expr::*;

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

impl<'a> Parser<'a> {
    pub (super) fn parse_expr(&mut self, min_bp: usize) -> (Option<Expr>, Vec<SpectrError>) {
        if let Some(_) = self.peek_type() {
            let &tok = self.next().unwrap();
            let (mut opt, mut errs) = self.nud(&tok);
            if let Some(expr) = opt.clone() {
                while let Some(next_tok) = self.peek() {
                    if lbp(next_tok.token_type) <= min_bp {
                        break;
                    }

                    let (next_opt, mut next_errs) = self.led(&expr);
                    opt = next_opt;
                    errs.append(&mut next_errs);
                }
            }

            return (opt, errs)
        }

        (None, Vec::new())
    }

    fn parse_paren(&mut self, start: usize) -> (Option<Expr>, Vec<SpectrError>) {
        // let elements: Vec<SBox<Expr>> = Vec::new();
        let (opt, mut errs) = self.parse_expr(0);
        if let Some(expr) = opt {
            match self.peek_type() {
                Some(TokenType::RParen) => {
                    self.next();
                    return (Some(expr), errs);
                },
                Some(TokenType::Comma) => {
                    let comma_range = self.peek().unwrap().source_range;
                    while let Some(&tt) = self.next() {
                        if tt.token_type == TokenType::RParen {
                            break;
                        }
                    }

                    errs.push(SpectrError {
                        error_type: SpectrErrorType::ParserError,
                        src: Some(comma_range),
                        msg: "tuples not yet supported".to_string()
                    });

                    return (Some(expr), errs);
                }
                Some(_) => {
                    let range = self.peek().unwrap().source_range;
                    while let Some(&tt) = self.next() {
                        if tt.token_type == TokenType::RParen {
                            break;
                        }
                    }

                    errs.push(SpectrError {
                        error_type: SpectrErrorType::ParserError,
                        src: Some(range),
                        msg: "unexpected token inside parentheses".to_string()
                    });

                    return (Some(expr), errs);
                },
                None => {
                    errs.push(SpectrError {
                        error_type: SpectrErrorType::ParserError,
                        src: Some(expr.span),
                        msg: "unclosed parentheses".to_string()
                    });
                    return (Some(expr), errs);
                }
            }
        }
        else {
            match self.peek_type() {
                Some(TokenType::RParen) => {
                    let end_range = self.next().unwrap().source_range;
                    let range = SourceRange {
                        start,
                        length: end_range.start - start + end_range.length
                    };

                    return (
                        Some(Expr::new(ExprShape::UnitExpr, range)),
                        errs
                    );
                },
                Some(_) => {
                    let range = self.peek().unwrap().source_range;
                    while let Some(&tt) = self.next() {
                        if tt.token_type == TokenType::RParen {
                            break;
                        }
                    }

                    errs.push(SpectrError {
                        error_type: SpectrErrorType::ParserError,
                        src: Some(range),
                        msg: "unexpected token inside parentheses".to_string()
                    });

                    return (None, errs);
                },
                None => {
                    errs.push(SpectrError {
                        error_type: SpectrErrorType::ParserError,
                        src: Some(SourceRange { start, length: 1 }),
                        msg: "unclosed parentheses".to_string()
                    });

                    return (None, errs);
                }
            }
        }
    }

    fn nud(&mut self, tok: &Token) -> (Option<Expr>, Vec<SpectrError>) {
        match tok.token_type {
            TokenType::Identifier => (
                Some(Expr::new(
                    ExprShape::IdentifierExpr(Identifier { name: tok.text.to_string() }),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::NIL => (
                Some(Expr::new(
                    ExprShape::NilLiteralExpr,
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::TRUE => (
                Some(Expr::new(
                    ExprShape::BoolLiteralExpr(true),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::FALSE => (
                Some(Expr::new(
                    ExprShape::BoolLiteralExpr(false),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::IntLiteral => (
                Some(Expr::new(
                    ExprShape::IntLiteralExpr(tok.text.parse().unwrap()),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::FloatLiteral => (
                Some(Expr::new(
                    ExprShape::FloatLiteralExpr(tok.text.parse().unwrap()),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::StringLiteral => (
                Some(Expr::new(
                    ExprShape::StrLiteralExpr(tok.text['"'.len_utf8()..(tok.text.len() - '"'.len_utf8())].to_string()),
                    tok.source_range
                )),
                Vec::new()
            ),
            TokenType::LParen => self.parse_paren(tok.source_range.start),
            _ => (None, Vec::new()),
        }
    }

    fn led(&mut self, left: &Expr) -> (Option<Expr>, Vec<SpectrError>) {
        let &tok = self.next().unwrap();
        if let Some(op) = BinaryOp::from_token(tok.token_type) {
            let (opt, mut errs) = self.parse_expr(rbp(tok.token_type));
            if let Some(expr) = opt {
                let span = SourceRange::span_two(left.span, expr.span);
                (Some(Expr::new(ExprShape::BinaryExpr(op, left.clone(), expr), span)), errs)
            }
            else {
                errs.push(SpectrError {
                    error_type: SpectrErrorType::ParserError,
                    src: Some(tok.source_range),
                    msg: "couldn't parse expression to the right of binary operator".to_string()
                });
                (None, errs)
            }
        }
        else {
            (None, vec![SpectrError {
                error_type: SpectrErrorType::ParserError,
                src: Some(tok.source_range),
                msg: "invalid token in led position".to_string()
            }])
        }
    }
}