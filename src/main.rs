mod error;
mod lexer;
mod parser;

use lexer::*;
use error::*;

use crate::parser::Parser;

fn main() {
    let file = File::read_file("demo.spectr").unwrap();
    let tokens = Lexer::tokenize_file(&file);
    Parser::parse(&tokens, &file);

    println!("Hello, world!");
}
