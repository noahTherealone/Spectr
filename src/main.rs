mod error;
mod lexer;
mod parser;
mod stmt;
mod expr;
mod kind;
mod r#type;
mod name_resolver;
mod kind_checker;
mod type_checker;

use colored::Colorize;
use lexer::*;
use error::*;

use crate::parser::Parser;

use std::io::Write;

fn main() {
    let file = File::read_file("demo.spectr").unwrap();
    let tokens = Lexer::tokenize_file(&file);

    let mut msg = "┌╴Tokens>\n".bright_purple().bold().to_string();
    for token in tokens.clone() {
        msg += token.text;
        msg += &" | ".bright_black().to_string();
    }
    msg = msg.replace("\n", &"\n│ ".bright_purple().bold().to_string());
    msg += &"\n└───────────".bright_purple().bold().to_string();
    println!("{}", msg);

    std::io::stdout().flush().unwrap();

    Parser::parse(&tokens, &file);

    println!("Hello, world!");
}
