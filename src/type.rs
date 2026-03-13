use std::collections::BTreeSet;



#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum BasicType {
    Void,
    Bool,
    Int,
    Float,
    Str,

    Any,

    Type,
    Kind,
}

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Type {
    Basic(BasicType),
    List(Box<Type>),
    Tuple(Vec<Type>),
    Union(BTreeSet<Type>),
    Inter(BTreeSet<Type>),
    Map(Box<Type>, Box<Type>),
}

impl Type {

}