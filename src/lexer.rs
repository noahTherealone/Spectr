use strum_macros::EnumIter;
use std::collections::HashMap;
use once_cell::sync::Lazy;

use crate::error::*;
use colored::*;

#[derive(Debug, EnumIter, Clone, Copy, Eq, PartialEq, Hash)]
pub enum TokenType {
    Identifier,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    Plus,
    Minus,
    Star,
    Slash,
    Modulus,
    Ampersand,
    Bar,
    Equals,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    RightArrow,
    DoubleRightArrow,
    Colon,
    DoubleColon,

    Assign,
    ColonAssign,
    DoubleColonAssign,

    KIND,
    TYPE,
    VALUE,

    TRUE,
    FALSE,
    NIL,

    AND,
    OR,
    NOT,
    XOR,

    IF,
    ELIF,
    ELSE,
    WHILE,
    FOR,
    IN,
    FUN,

    LnBreak,
}

static TOKEN_KEYWORDS: &[(&str, TokenType)] = &[
    ("kind",  TokenType::KIND),
    ("type",  TokenType::TYPE),
    ("value", TokenType::VALUE),
    ("true",  TokenType::TRUE),
    ("false", TokenType::FALSE),
    ("nil",   TokenType::NIL),
    ("and",   TokenType::AND),
    ("or",    TokenType::OR),
    ("not",   TokenType::NOT),
    ("xor",   TokenType::XOR),
    ("if",    TokenType::IF),
    ("elif",  TokenType::ELIF),
    ("else",  TokenType::ELSE),
    ("while", TokenType::WHILE),
    ("for",   TokenType::FOR),
    ("in",    TokenType::IN),
    ("fun",   TokenType::FUN),
];

static KEYWORD_TO_TOKEN_TYPE: Lazy<HashMap<&'static str, TokenType>> = Lazy::new(
    || TOKEN_KEYWORDS.iter().cloned().collect()
);

static TOKEN_TYPE_TO_KEYWORD: Lazy<HashMap<TokenType, &'static str>> = Lazy::new(
    || TOKEN_KEYWORDS.iter().map(|(a, b)| (*b, *a)).collect()
);

static TOKEN_SYMBOLS: &[(&str, TokenType)] = &[
    ("+",   TokenType::Plus),
    ("-",   TokenType::Minus),
    ("*",   TokenType::Star),
    ("/",   TokenType::Slash),
    ("%",   TokenType::Modulus),
    ("&",   TokenType::Ampersand),
    ("|",   TokenType::Bar),
    ("==",  TokenType::Equals),
    ("<",   TokenType::Less),
    ("<=",  TokenType::LessEqual),
    (">",   TokenType::Greater),
    (">=",  TokenType::GreaterEqual),
    ("(",   TokenType::LParen),
    (")",   TokenType::RParen),
    ("{",   TokenType::LBrace),
    ("}",   TokenType::RBrace),
    ("[",   TokenType::RBracket),
    ("]",   TokenType::RBracket),
    (",",   TokenType::Comma),
    ("->",  TokenType::RightArrow),
    ("=>",  TokenType::DoubleRightArrow),
    (":",   TokenType::Colon),
    ("::",  TokenType::DoubleColon),
    ("=",   TokenType::Assign),
    (":=",  TokenType::ColonAssign),
    ("::=", TokenType::DoubleColonAssign)
];

static SYMBOL_TO_TOKEN_TYPE: Lazy<HashMap<&'static str, TokenType>> = Lazy::new(
    || TOKEN_KEYWORDS.iter().cloned().collect()
);

static TOKEN_TYPE_TO_SYMBOL: Lazy<HashMap<TokenType, &'static str>> = Lazy::new(
    || TOKEN_KEYWORDS.iter().map(|(a, b)| (*b, *a)).collect()
);

impl TokenType {
    pub fn from_keyword<'a>(kw: &'a str) -> TokenType {
        match KEYWORD_TO_TOKEN_TYPE.get(kw) {
            Some(&token_type) => token_type,
            None => TokenType::Identifier,
        }
    }

    pub fn to_keyword(&self) -> Option<&'static str> {
        TOKEN_TYPE_TO_KEYWORD.get(self).copied()
    }
}

fn make_word_token<'a>(word: &'a str, start: usize, length: usize) -> Token<'a> {
    let token_type = TokenType::from_keyword(word);
    Token { token_type, text: word, source_range: SourceRange { start, length } }
}

#[derive(Debug, Clone, Copy)]
pub struct Token<'a> {
    pub token_type: TokenType,
    pub text: &'a str,
    pub source_range: SourceRange
}

pub struct Lexer<'a> {
    file: &'a File<'a>,
    tokens: Vec<Token<'a>>,
    index: usize,
    pos: usize
}

impl<'a> Lexer<'a> {
    fn new(file: &'a File) -> Lexer<'a> {
        Lexer {
            file,
            tokens: Vec::new(),
            index: 0,
            pos: 0,
        }
    }

    fn peek(&self) -> Option<char> {
        self.file.code[self.pos..].chars().next()
    }

    fn bump(&mut self) {
        if let Some(ch) = self.peek() {
            self.pos += ch.len_utf8();
            self.index += 1;
        }
    }

    fn next(&mut self) -> Option<char> {
        let ch = self.peek()?;
        self.bump();
        Some(ch)
    }

    fn tokenize_number(&mut self) -> Result<Token<'a>, SpectrError> {
        let start_pos = self.pos;
        let start_index = self.index;
        let mut found_dot = false;

        while let Some(ch) = self.peek() {
            if ch == '.' {
                if found_dot {
                    return Err(SpectrError {
                        error_type: SpectrErrorType::LexerError,
                        src: Some(SourceRange {
                            start: start_index,
                            length: self.index - start_index,
                        }),
                        msg: "found two dots inside numeric literal".to_string(),
                    })
                }

                found_dot = true;
                continue;
            }

            if !ch.is_numeric() { break; }
            self.next();
        }

        let literal: &str = &self.file.code[start_pos..self.pos];

        Ok(Token {
            token_type:
                if found_dot { TokenType::FloatLiteral }
                else { TokenType::IntLiteral },
            text: literal,
            source_range: SourceRange {
                start: start_index,
                length: self.index - start_index
            },
        })
    }

    fn tokenize_word(&mut self) -> Token<'a> {
        let start_pos = self.pos;
        let start_index = self.index;

        while let Some(ch) = self.next() {
            if !ch.is_alphanumeric() { break; }
        }

        let word: &str = &self.file.code[start_pos..self.pos];

        make_word_token(word, start_index, self.index - start_index)
    }

    fn tokenize_symbol(&mut self) -> Result<Token<'a>, SpectrError> {
        let mut candidates: Vec<(&str, TokenType)> = TOKEN_SYMBOLS.iter().cloned().collect();
        let start_pos = self.pos;
        let start_index = self.index;
        while let Some(_) = self.next() {
            let current_slice = &self.file.code[start_pos..self.pos];
            candidates.retain(
                |&(sy, _)| sy.starts_with(current_slice)
            );

            if candidates.is_empty() {
                return Err(SpectrError {
                    error_type: SpectrErrorType::LexerError,
                    src: Some(SourceRange { start: start_index, length: self.index - start_index }),
                    msg: "couldn't match symbol".to_string(),
                });
            }

            if candidates.len() == 1 {
                let res = candidates[0];
                self.pos = start_pos + res.0.len();
                self.index = start_index + res.0.chars().count();
                return Ok(Token {
                    token_type: res.1,
                    text: res.0,
                    source_range: SourceRange {
                        start: start_index,
                        length: self.index - start_index
                    }
                });
            }
        }

        Err(SpectrError {
            error_type: SpectrErrorType::LexerError,
            src: Some(SourceRange { start: start_index, length: self.index - start_index }),
            msg: "couldn't match symbol".to_string(),
        })
    }

    pub fn tokenize_file(file: &'a File<'a>) -> Vec<Token<'a>> {
        let mut lexer = Lexer::new(file);

        while let Some(ch) = lexer.peek() {
            if ch == '\n' {
                lexer.tokens.push(Token {
                    token_type: TokenType::LnBreak,
                    text: "\n",
                    source_range: SourceRange { start: lexer.index, length: 1 }
                });

                lexer.next();
                continue;
            }

            if ch.is_whitespace() {
                lexer.next();
                continue;
            }

            if ch.is_numeric() {
                match lexer.tokenize_number() {
                    Ok(token) => lexer.tokens.push(token),
                    Err(err) => eprintln!("{}", err.display(lexer.file).red()),
                }
                
                continue;
            }

            if ch.is_alphabetic() {
                let token = lexer.tokenize_word();
                lexer.tokens.push(token);
                continue;
            }

            match lexer.tokenize_symbol() {
                Ok(token) => lexer.tokens.push(token),
                Err(err) => eprintln!("{}", err.display(lexer.file)),
            }
        }

        lexer.tokens.clone()
    }

}