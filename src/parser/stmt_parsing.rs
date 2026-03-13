
use super::Parser;
use super::expr_parsing;
use crate::error::*;
use crate::lexer::{Token, TokenType};
use crate::expr::{Expr, ExprShape};
use crate::stmt::*;

fn wrap_first<A, B>((a, b): (A, B)) -> (Option<A>, B) {
    (Some(a), b)
}

fn wrap_result<A, B>(a: Result<A, Vec<B>>) -> (Option<A>, Vec<B>) {
    match a {
        Ok(ok) => (Some(ok), Vec::new()),
        Err(errs) => (None, errs)
    }
}

impl<'a> Parser<'a> {
    pub(super) fn parse_stmt(&mut self) -> (Option<Stmt>, Vec<SpectrError>) {
        let tt = self.peek().map(|t| t.token_type);
        let _file = self.file;
        match tt {
            Some(TokenType::LnBreak) => {
                self.next();
                self.parse_stmt()
            }
            Some(TokenType::IF) => wrap_first(self.parse_if_stmt()),
            Some(TokenType::WHILE) => wrap_result(self.parse_while_stmt()),
            Some(_) => {
                let (opt, mut errs) = self.parse_expr(0);
                if opt == None {
                    return (None, errs);
                }

                let expr = opt.unwrap();
                let span = expr.span;
                let wrapped = Some(Stmt::new(StmtShape::ExprStmt(expr), span));

                let next_tt = self.peek_type();
                match next_tt {
                    Some(TokenType::LnBreak) => (wrapped, errs),
                    Some(_) => {
                        errs.push(SpectrError {
                            error_type: SpectrErrorType::ParserError,
                            src: Some(span),
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


    fn parse_if_stmt(&mut self) -> (Stmt, Vec<SpectrError>) {
        (Stmt::new(
            StmtShape::IfElseStmt(
                Expr::new(ExprShape::BoolLiteralExpr(true), SourceRange { start: 0, length: 0 }),
                Vec::new(), Vec::new()
            ),
            SourceRange { start: 0, length: 0 }
        ), Vec::new())
    }

    fn parse_while_stmt(&mut self) -> Result<Stmt, Vec<SpectrError>> {
        let &while_token = self.next().unwrap();
        let (cond_opt, mut errs) = self.parse_expr(0);
        if let Some(cond) = cond_opt {
            let span = SourceRange { start: while_token.source_range.start, length: cond.span.start - while_token.source_range.start + cond.span.length };

            // parse block here later

            Ok(Stmt::new(StmtShape::WhileStmt(cond, Vec::new()), span))
        }
        else {
            errs.push(SpectrError {
                error_type: SpectrErrorType::ParserError,
                src: Some(while_token.source_range),
                msg: "couldn't parse condition for while-statement".to_string()
            });

            Err(errs)
        }
    }
}