
use std::usize;

use crate::error::*;
use crate::lexer::*;
use crate::expr::*;
use crate::stmt::*;

use colored::*;

pub struct Parser<'a> {
    file: &'a File<'a>,
    tokens: &'a [Token<'a>],
    pos: usize,
}

mod stmt_parsing;
mod expr_parsing;

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

    pub fn parse(tokens: &'a [Token<'a>], file: &'a File<'a>) -> Vec<Stmt> {
        let mut parser = Parser::new(tokens, file);
        let mut stmts: Vec<Stmt> = Vec::new();

        loop {
            let (opt, errs) = parser.parse_stmt();

            for err in errs {
                eprintln!("{}", err.display(parser.file).red());
            }

            if let Some(stmt) = opt {
                stmts.push(stmt);
            } else {
                return stmts;
            }
        }
    }
}