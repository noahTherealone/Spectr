
use crate::r#type::Type;

#[derive(Debug, Clone, PartialEq)]
pub enum Kind {
    Value(Option<Type>), // type checker needs to fill this
    Type,
    Kind,
    Functor(Box<Kind>, Box<Kind>)
}