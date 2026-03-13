use std::fmt::Debug;
use std::fs;
use std::io::Error;
use std::ops::Deref;

use strum_macros::Display;

#[derive(Debug, Clone)]
pub struct File<'a> {
    pub path: &'a str,
    pub code: String,
    line_breaks: Vec<usize>,
}

impl<'a> File<'a> {
    pub fn new(path: &'a str, code: String) -> File<'a> {
        let mut line_breaks: Vec<usize> = Vec::new();
        let mut chars = code.chars();
        let mut index: usize = 0;
        while let Some(ch) = chars.next() {
            if ch == '\n' {
                line_breaks.push(index);
            }

            index += 1;
        }

        File {
            path,
            code,
            line_breaks,
        }
    }

    pub fn read_file(path: &'a str) -> Result<File<'a>, Error> {
        let file = fs::read_to_string(path);
        match file {
            Ok(code) => Ok(File::new(path, code)),
            Err(err) => Err(err),
        }
    }

    pub fn get_position(&self, index: usize) -> (usize, usize) {
        if self.line_breaks.len() == 0 {
            return (1, index+1);
        }

        let mut line = 1;
        while line <= self.line_breaks.len() && self.line_breaks[line-1] < index {
            line += 1;
        }

        (line, index + 1 - self.line_breaks[line-1])
    }

    pub fn display_position(&self, index: usize) -> String {
        let pos = self.get_position(index);
        format!("{} ({}:{})", self.path, pos.0, pos.1)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct SourceRange {
    pub start: usize,
    pub length: usize
}

impl SourceRange {
    pub fn span_two(first: SourceRange, second: SourceRange) -> SourceRange {
        SourceRange { start: first.start, length: second.start - first.start + second.length }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct SBox<T> {
    pub span: Option<SourceRange>,
    pub node: Box<T>,
}

impl<T> Deref for SBox<T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        self.node.as_ref()
    }
}

impl<T> SBox<T> {
    pub fn new(node: T, span: Option<SourceRange>) -> SBox<T> {
        SBox {
            span,
            node: Box::new(node)
        }
    }
}

#[derive(Debug, Clone, Copy, Display)]
pub enum SpectrErrorType {
    UnintendedError,
    LexerError,
    ParserError,
}

#[derive(Debug, Clone)]
pub struct SpectrError {
    pub error_type: SpectrErrorType,
    pub src: Option<SourceRange>,
    pub msg: String,
}

impl SpectrError {
    pub fn display<'a>(&self, file: &'a File<'a>) -> String {
        match self.src {
            Some(span) => format!(
                "{} at {}: {}",
                self.error_type,
                file.display_position(span.start),
                self.msg
            ),
            None => format!(
                "{} from unknown source: {}",
                self.error_type,
                self.msg
            ),
        }
    }
}