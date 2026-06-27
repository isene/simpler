//! The Simpler bootstrap compiler — milestone M5a.
//!
//! Pipeline: lex -> parse -> emit C -> system `cc`. M5a adds the first half of
//! the object/type form: **records**, user-defined product types.
//!
//!   Point = type {
//!     x : Int
//!     y : Int
//!   }
//!
//! A record is constructed with named fields (`Point(x = 3, y = 4)`), read with
//! a field send (`p.x`), and has value semantics (a C struct, copied on bind /
//! pass / return). Fields are `Int`/`Str`/`Bool` for now; nested records,
//! variants + `match`, and collections come in M5b+.
//!
//! Earlier milestones still hold: capabilities and effects are compile-time
//! only, `?` propagates failure, `fmt` is canonical, `test` runs `test_*`.

use std::collections::HashMap;
use std::env;
use std::fs;
use std::path::Path;
use std::process::{exit, Command};
use std::sync::OnceLock;

#[derive(Debug)]
struct CErr {
    line: u32,
    msg: String,
}

fn ce(line: u32, msg: impl Into<String>) -> CErr {
    CErr { line, msg: msg.into() }
}

// ----------------------------- Types & effects -----------------------------

#[derive(Debug, Clone, Copy, PartialEq)]
enum Ty {
    Int,
    Str,
    Bool,
    Sys,
    Screen,
    Files,
    Mail,
    /// A user-defined record type, named by its (leaked, so `Copy`) name.
    User(&'static str),
}

/// Value types have a C representation and value semantics; capabilities are
/// erased at runtime. A record is a value (a C struct).
fn is_value(t: Ty) -> bool {
    matches!(t, Ty::Int | Ty::Str | Ty::Bool | Ty::User(_))
}

fn cty(t: Ty) -> &'static str {
    match t {
        Ty::Int => "long",
        Ty::Str => "const char *",
        Ty::Bool => "int",
        Ty::User(name) => name,
        _ => unreachable!("capabilities have no C type"),
    }
}

fn cap_member(recv: Ty, name: &str) -> Option<Ty> {
    match (recv, name) {
        (Ty::Sys, "screen") => Some(Ty::Screen),
        (Ty::Sys, "files") => Some(Ty::Files),
        (Ty::Sys, "mail") => Some(Ty::Mail),
        _ => None,
    }
}

#[derive(Debug, Clone, Copy, Default)]
struct Effects {
    io: bool,
    fail: bool,
}

impl Effects {
    fn all() -> Effects { Effects { io: true, fail: true } }
    fn io_fail() -> Effects { Effects { io: true, fail: true } }
    fn union(&mut self, o: Effects) {
        self.io |= o.io;
        self.fail |= o.fail;
    }
    fn covered_by(&self, decl: Effects) -> bool {
        (!self.io || decl.io) && (!self.fail || decl.fail)
    }
}

// ----------------------------- AST -----------------------------------------

#[derive(Debug, Clone, Copy)]
enum BinOp {
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    Lt,
    Gt,
}

fn op_sym(op: BinOp) -> &'static str {
    match op {
        BinOp::Add => "+",
        BinOp::Sub => "-",
        BinOp::Mul => "*",
        BinOp::Div => "/",
        BinOp::Eq => "==",
        BinOp::Lt => "<",
        BinOp::Gt => ">",
    }
}

#[derive(Debug, Clone)]
struct Arg {
    name: Option<String>,
    value: Expr,
}

#[derive(Debug, Clone)]
enum Expr {
    Int(i64),
    Str(String),
    Var(String),
    Bin { op: BinOp, lhs: Box<Expr>, rhs: Box<Expr> },
    Call { name: String, args: Vec<Arg> },
    Send {
        recv: Box<Expr>,
        name: String,
        args: Vec<Arg>,
        parens: bool,
        block: Option<Block>,
    },
    Try(Box<Expr>),
}

#[derive(Debug, Clone)]
struct Block {
    param: Option<String>,
    body: Vec<Stmt>,
}

#[derive(Debug, Clone)]
struct Stmt {
    line: u32,
    kind: SKind,
}

#[derive(Debug, Clone)]
enum SKind {
    Bind { name: String, ty: Option<Ty>, value: Expr },
    If { cond: Expr, then: Vec<Stmt>, els: Vec<Stmt> },
    Expr(Expr),
}

#[derive(Debug)]
struct Func {
    name: String,
    params: Vec<(String, Option<Ty>)>,
    ret: Option<Ty>,
    effects: Effects,
    body: Vec<Stmt>,
    line: u32,
}

/// A record type definition: named fields, in declaration order.
#[derive(Debug, Clone)]
struct RecordDef {
    name: &'static str,
    fields: Vec<(String, Ty)>,
    line: u32,
}

/// A top-level item, kept in source order so the formatter can preserve it.
#[derive(Debug)]
enum Item {
    Func(Func),
    Type(RecordDef),
}

struct Sig {
    params: Vec<(String, Ty)>,
    ret: Option<Ty>,
    effects: Effects,
    is_main: bool,
}

/// The record types of the program being compiled. Set once before emit, then
/// read by construction and field access, so the type table need not thread
/// through every emit function.
static TYPES: OnceLock<Vec<RecordDef>> = OnceLock::new();

fn types() -> &'static [RecordDef] {
    TYPES.get().map(|v| v.as_slice()).unwrap_or(&[])
}

fn find_record(name: &str) -> Option<&'static RecordDef> {
    types().iter().find(|r| r.name == name)
}

// ----------------------------- Lexer ---------------------------------------

#[derive(Debug, Clone, PartialEq)]
enum Tok {
    Ident(String),
    Str(String),
    IntLit(i64),
    Dot,
    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Assign,
    EqEq,
    Colon,
    Bang,
    Question,
    Plus,
    Minus,
    Star,
    Slash,
    Lt,
    Gt,
    Eof,
}

fn lex(src: &str) -> Result<(Vec<(Tok, u32)>, Vec<Cmt>), CErr> {
    let cs: Vec<char> = src.chars().collect();
    let mut i = 0;
    let mut line: u32 = 1;
    let mut toks: Vec<(Tok, u32)> = Vec::new();
    let mut comments: Vec<Cmt> = Vec::new();
    while i < cs.len() {
        let c = cs[i];
        match c {
            '\n' => { line += 1; i += 1; }
            ' ' | '\t' | '\r' => i += 1,
            '/' if i + 1 < cs.len() && cs[i + 1] == '/' => {
                let trailing = toks.last().map_or(false, |t| t.1 == line);
                i += 2;
                let start = i;
                while i < cs.len() && cs[i] != '\n' {
                    i += 1;
                }
                let text: String = cs[start..i].iter().collect();
                comments.push(Cmt { line, trailing, text: text.trim().to_string() });
            }
            '.' => { toks.push((Tok::Dot, line)); i += 1; }
            '(' => { toks.push((Tok::LParen, line)); i += 1; }
            ')' => { toks.push((Tok::RParen, line)); i += 1; }
            '{' => { toks.push((Tok::LBrace, line)); i += 1; }
            '}' => { toks.push((Tok::RBrace, line)); i += 1; }
            ',' => { toks.push((Tok::Comma, line)); i += 1; }
            '!' => { toks.push((Tok::Bang, line)); i += 1; }
            '?' => { toks.push((Tok::Question, line)); i += 1; }
            '+' => { toks.push((Tok::Plus, line)); i += 1; }
            '-' => { toks.push((Tok::Minus, line)); i += 1; }
            '*' => { toks.push((Tok::Star, line)); i += 1; }
            '/' => { toks.push((Tok::Slash, line)); i += 1; }
            '<' => { toks.push((Tok::Lt, line)); i += 1; }
            '>' => { toks.push((Tok::Gt, line)); i += 1; }
            ':' => { toks.push((Tok::Colon, line)); i += 1; }
            '=' => {
                if i + 1 < cs.len() && cs[i + 1] == '=' {
                    toks.push((Tok::EqEq, line));
                    i += 2;
                } else {
                    toks.push((Tok::Assign, line));
                    i += 1;
                }
            }
            '"' => {
                let start_line = line;
                i += 1;
                let mut s = String::new();
                while i < cs.len() && cs[i] != '"' {
                    if cs[i] == '\n' {
                        line += 1;
                        s.push('\n');
                        i += 1;
                    } else if cs[i] == '\\' && i + 1 < cs.len() {
                        i += 1;
                        s.push(match cs[i] {
                            'n' => '\n',
                            't' => '\t',
                            'r' => '\r',
                            '"' => '"',
                            '\\' => '\\',
                            o => o,
                        });
                        i += 1;
                    } else {
                        s.push(cs[i]);
                        i += 1;
                    }
                }
                if i >= cs.len() {
                    return Err(ce(start_line, "unterminated string literal"));
                }
                i += 1;
                toks.push((Tok::Str(s), start_line));
            }
            c if c.is_ascii_digit() => {
                let start = i;
                while i < cs.len() && cs[i].is_ascii_digit() {
                    i += 1;
                }
                let lit: String = cs[start..i].iter().collect();
                let n = lit.parse::<i64>().map_err(|_| ce(line, format!("bad number `{}`", lit)))?;
                toks.push((Tok::IntLit(n), line));
            }
            c if c.is_alphabetic() || c == '_' => {
                let start = i;
                while i < cs.len() && (cs[i].is_alphanumeric() || cs[i] == '_') {
                    i += 1;
                }
                toks.push((Tok::Ident(cs[start..i].iter().collect()), line));
            }
            other => return Err(ce(line, format!("unexpected character '{}'", other))),
        }
    }
    toks.push((Tok::Eof, line));
    Ok((toks, comments))
}

// ----------------------------- Parser --------------------------------------

struct Parser {
    toks: Vec<(Tok, u32)>,
    pos: usize,
    no_block: bool,
    /// User type names, mapped to a leaked `&'static str` so `Ty::User` stays
    /// `Copy`. Filled by a pre-scan so forward references resolve.
    type_names: HashMap<String, &'static str>,
}

impl Parser {
    fn new(toks: Vec<(Tok, u32)>) -> Self {
        Parser { toks, pos: 0, no_block: false, type_names: HashMap::new() }
    }

    fn peek(&self) -> &Tok { &self.toks[self.pos].0 }
    fn cur_line(&self) -> u32 { self.toks[self.pos].1 }
    fn at(&self, n: usize) -> Option<&Tok> { self.toks.get(self.pos + n).map(|t| &t.0) }

    fn bump(&mut self) -> Tok {
        let t = self.toks[self.pos].0.clone();
        self.pos += 1;
        t
    }

    fn eat(&mut self, t: &Tok) -> Result<(), CErr> {
        if self.peek() == t {
            self.pos += 1;
            Ok(())
        } else {
            Err(ce(self.cur_line(), format!("expected {:?}, found {:?}", t, self.peek())))
        }
    }

    fn ident(&mut self) -> Result<String, CErr> {
        let line = self.cur_line();
        match self.bump() {
            Tok::Ident(s) => Ok(s),
            t => Err(ce(line, format!("expected identifier, found {:?}", t))),
        }
    }

    fn ident_is(&self, s: &str) -> bool {
        matches!(self.peek(), Tok::Ident(n) if n == s)
    }

    /// Register every `Name = type` so field and parameter types can refer to a
    /// record defined later in the file.
    fn prescan_types(&mut self) {
        let n = self.toks.len();
        let mut i = 0;
        while i + 2 < n {
            if let (Tok::Ident(name), Tok::Assign, Tok::Ident(kw)) =
                (&self.toks[i].0, &self.toks[i + 1].0, &self.toks[i + 2].0)
            {
                if kw == "type" && !self.type_names.contains_key(name) {
                    let leaked: &'static str = Box::leak(name.clone().into_boxed_str());
                    self.type_names.insert(name.clone(), leaked);
                }
            }
            i += 1;
        }
    }

    fn program(&mut self) -> Result<Vec<Item>, CErr> {
        self.prescan_types();
        let mut items = Vec::new();
        while *self.peek() != Tok::Eof {
            if matches!(self.peek(), Tok::Ident(_)) && self.at(1) == Some(&Tok::Assign) {
                items.push(Item::Type(self.typedef()?));
            } else {
                items.push(Item::Func(self.func()?));
            }
        }
        Ok(items)
    }

    /// typedef := ident '=' 'type' '{' (ident ':' Type)* '}'
    fn typedef(&mut self) -> Result<RecordDef, CErr> {
        let line = self.cur_line();
        let name = self.ident()?;
        self.eat(&Tok::Assign)?;
        if !self.ident_is("type") {
            return Err(ce(self.cur_line(), "expected `type` after `=` in a type definition"));
        }
        self.pos += 1;
        self.eat(&Tok::LBrace)?;
        let mut fields = Vec::new();
        while *self.peek() != Tok::RBrace && *self.peek() != Tok::Eof {
            let fname = self.ident()?;
            self.eat(&Tok::Colon)?;
            let fl = self.cur_line();
            let ft = self.parse_type()?;
            if !matches!(ft, Ty::Int | Ty::Str | Ty::Bool) {
                return Err(ce(fl, "record fields must be Int, Str, or Bool for now (nested types come later)"));
            }
            fields.push((fname, ft));
            if *self.peek() == Tok::Comma {
                self.pos += 1;
            }
        }
        self.eat(&Tok::RBrace)?;
        let leaked = self.type_names[&name];
        Ok(RecordDef { name: leaked, fields, line })
    }

    fn func(&mut self) -> Result<Func, CErr> {
        let line = self.cur_line();
        let name = self.ident()?;
        self.eat(&Tok::LParen)?;
        let mut params = Vec::new();
        if *self.peek() != Tok::RParen {
            loop {
                let pname = self.ident()?;
                let pty = if *self.peek() == Tok::Colon {
                    self.pos += 1;
                    Some(self.parse_type()?)
                } else {
                    None
                };
                params.push((pname, pty));
                if *self.peek() == Tok::Comma { self.pos += 1; } else { break; }
            }
        }
        self.eat(&Tok::RParen)?;
        let ret = if *self.peek() == Tok::Colon {
            self.pos += 1;
            Some(self.parse_type()?)
        } else {
            None
        };
        let mut effects = Effects::default();
        while *self.peek() == Tok::Bang {
            self.pos += 1;
            let l = self.cur_line();
            let e = self.ident()?;
            match e.as_str() {
                "IO" => effects.io = true,
                "Fail" => effects.fail = true,
                _ => return Err(ce(l, format!("unknown effect `!{}` (coarse set is !IO, !Fail)", e))),
            }
        }
        let body = self.stmt_block()?;
        Ok(Func { name, params, ret, effects, body, line })
    }

    fn stmt_block(&mut self) -> Result<Vec<Stmt>, CErr> {
        self.eat(&Tok::LBrace)?;
        let mut body = Vec::new();
        while *self.peek() != Tok::RBrace && *self.peek() != Tok::Eof {
            body.push(self.stmt()?);
        }
        self.eat(&Tok::RBrace)?;
        Ok(body)
    }

    fn stmt(&mut self) -> Result<Stmt, CErr> {
        let line = self.cur_line();
        if self.ident_is("if") {
            return self.if_stmt();
        }
        if let Tok::Ident(name) = self.peek().clone() {
            match self.at(1) {
                Some(Tok::Assign) => {
                    self.pos += 2;
                    let value = self.expr()?;
                    return Ok(Stmt { line, kind: SKind::Bind { name, ty: None, value } });
                }
                Some(Tok::Colon) => {
                    self.pos += 2;
                    let ty = self.parse_type()?;
                    self.eat(&Tok::Assign)?;
                    let value = self.expr()?;
                    return Ok(Stmt { line, kind: SKind::Bind { name, ty: Some(ty), value } });
                }
                _ => {}
            }
        }
        Ok(Stmt { line, kind: SKind::Expr(self.expr()?) })
    }

    fn if_stmt(&mut self) -> Result<Stmt, CErr> {
        let line = self.cur_line();
        self.pos += 1;
        self.no_block = true;
        let cond = self.expr()?;
        self.no_block = false;
        let then = self.stmt_block()?;
        let els = if self.ident_is("else") {
            self.pos += 1;
            if self.ident_is("if") {
                vec![self.if_stmt()?]
            } else {
                self.stmt_block()?
            }
        } else {
            Vec::new()
        };
        Ok(Stmt { line, kind: SKind::If { cond, then, els } })
    }

    fn parse_type(&mut self) -> Result<Ty, CErr> {
        let line = self.cur_line();
        let n = self.ident()?;
        match n.as_str() {
            "Int" => Ok(Ty::Int),
            "Str" => Ok(Ty::Str),
            "Bool" => Ok(Ty::Bool),
            "Sys" => Ok(Ty::Sys),
            "Screen" => Ok(Ty::Screen),
            "Files" => Ok(Ty::Files),
            "Mail" => Ok(Ty::Mail),
            _ => match self.type_names.get(&n) {
                Some(&leaked) => Ok(Ty::User(leaked)),
                None => Err(ce(line, format!("unknown type `{}`", n))),
            },
        }
    }

    fn expr(&mut self) -> Result<Expr, CErr> { self.cmp() }

    fn cmp(&mut self) -> Result<Expr, CErr> {
        let mut e = self.add()?;
        loop {
            let op = match self.peek() {
                Tok::EqEq => BinOp::Eq,
                Tok::Lt => BinOp::Lt,
                Tok::Gt => BinOp::Gt,
                _ => break,
            };
            self.pos += 1;
            let rhs = self.add()?;
            e = Expr::Bin { op, lhs: Box::new(e), rhs: Box::new(rhs) };
        }
        Ok(e)
    }

    fn add(&mut self) -> Result<Expr, CErr> {
        let mut e = self.mul()?;
        loop {
            let op = match self.peek() {
                Tok::Plus => BinOp::Add,
                Tok::Minus => BinOp::Sub,
                _ => break,
            };
            self.pos += 1;
            let rhs = self.mul()?;
            e = Expr::Bin { op, lhs: Box::new(e), rhs: Box::new(rhs) };
        }
        Ok(e)
    }

    fn mul(&mut self) -> Result<Expr, CErr> {
        let mut e = self.postfix()?;
        loop {
            let op = match self.peek() {
                Tok::Star => BinOp::Mul,
                Tok::Slash => BinOp::Div,
                _ => break,
            };
            self.pos += 1;
            let rhs = self.postfix()?;
            e = Expr::Bin { op, lhs: Box::new(e), rhs: Box::new(rhs) };
        }
        Ok(e)
    }

    fn postfix(&mut self) -> Result<Expr, CErr> {
        let mut e = self.primary()?;
        while *self.peek() == Tok::Dot {
            self.pos += 1;
            let name = self.ident()?;
            let (args, parens, block) = if *self.peek() == Tok::LParen {
                (self.args()?, true, None)
            } else if *self.peek() == Tok::LBrace && !self.no_block {
                (Vec::new(), false, Some(self.block_lit()?))
            } else {
                (Vec::new(), false, None)
            };
            e = Expr::Send { recv: Box::new(e), name, args, parens, block };
        }
        if *self.peek() == Tok::Question {
            self.pos += 1;
            e = Expr::Try(Box::new(e));
        }
        Ok(e)
    }

    fn args(&mut self) -> Result<Vec<Arg>, CErr> {
        self.eat(&Tok::LParen)?;
        let mut args = Vec::new();
        if *self.peek() != Tok::RParen {
            loop {
                let name = if matches!(self.peek(), Tok::Ident(_)) && self.at(1) == Some(&Tok::Assign) {
                    let n = self.ident()?;
                    self.pos += 1;
                    Some(n)
                } else {
                    None
                };
                let value = self.expr()?;
                args.push(Arg { name, value });
                if *self.peek() == Tok::Comma { self.pos += 1; } else { break; }
            }
        }
        self.eat(&Tok::RParen)?;
        Ok(args)
    }

    fn block_lit(&mut self) -> Result<Block, CErr> {
        self.eat(&Tok::LBrace)?;
        let param = if let Tok::Ident(p) = self.peek().clone() {
            if matches!(self.at(1), Some(Tok::Ident(k)) if k == "in") {
                self.pos += 2;
                Some(p)
            } else {
                None
            }
        } else {
            None
        };
        let mut body = Vec::new();
        while *self.peek() != Tok::RBrace && *self.peek() != Tok::Eof {
            body.push(self.stmt()?);
        }
        self.eat(&Tok::RBrace)?;
        Ok(Block { param, body })
    }

    fn primary(&mut self) -> Result<Expr, CErr> {
        let line = self.cur_line();
        match self.peek().clone() {
            Tok::IntLit(n) => { self.pos += 1; Ok(Expr::Int(n)) }
            Tok::Str(s) => { self.pos += 1; Ok(Expr::Str(s)) }
            Tok::Ident(s) => {
                self.pos += 1;
                if *self.peek() == Tok::LParen {
                    let args = self.args()?;
                    Ok(Expr::Call { name: s, args })
                } else {
                    Ok(Expr::Var(s))
                }
            }
            Tok::LParen => {
                self.pos += 1;
                let e = self.expr()?;
                self.eat(&Tok::RParen)?;
                Ok(e)
            }
            t => Err(ce(line, format!("unexpected token {:?}", t))),
        }
    }
}

// ----------------------------- Scope ---------------------------------------

struct Scope {
    frames: Vec<HashMap<String, Ty>>,
}

impl Scope {
    fn new() -> Self { Scope { frames: vec![HashMap::new()] } }
    fn push(&mut self) { self.frames.push(HashMap::new()); }
    fn pop(&mut self) { self.frames.pop(); }
    fn declare(&mut self, name: String, ty: Ty) {
        self.frames.last_mut().unwrap().insert(name, ty);
    }
    fn lookup(&self, name: &str) -> Option<Ty> {
        self.frames.iter().rev().find_map(|f| f.get(name).copied())
    }
}

// ----------------------------- Emit C --------------------------------------

const RUNTIME: &str = "\
#include <stdio.h>
#include <stdlib.h>

/* Generated by the Simpler bootstrap compiler. Do not edit. */

static int simpler_failed = 0;

static void simpler_print_str(const char *s) { fputs(s, stdout); fputc('\\n', stdout); }
static void simpler_print_int(long n)        { printf(\"%ld\\n\", n); }
static void simpler_print_bool(int b)        { fputs(b ? \"true\" : \"false\", stdout); fputc('\\n', stdout); }

static const char *simpler_read(const char *path) {
    FILE *f = fopen(path, \"rb\");
    if (!f) { simpler_failed = 1; return \"\"; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); simpler_failed = 1; return \"\"; }
    size_t got = fread(buf, 1, (size_t)n, f);
    buf[got] = '\\0';
    fclose(f);
    return buf;
}

static void simpler_send(const char *to, const char *subject, const char *body) {
    printf(\"[mail] to=%s subject=%s\\n%s\\n\", to, subject, body);
}
";

fn build_sigs(funcs: &[&Func]) -> Result<HashMap<String, Sig>, CErr> {
    let mut sigs = HashMap::new();
    for f in funcs {
        let is_main = f.name == "main";
        let is_test = !is_main && f.name.starts_with("test_");
        let mut params = Vec::new();
        for (pn, pt) in &f.params {
            let ty = match pt {
                Some(t) => *t,
                None if is_main => Ty::Sys,
                None => {
                    return Err(ce(f.line, format!("parameter `{}` needs a type (e.g. `{} : Int`)", pn, pn)))
                }
            };
            params.push((pn.clone(), ty));
        }
        if is_main && f.ret.is_some() {
            return Err(ce(f.line, "`main` does not return a value"));
        }
        let mut effects = if is_main { Effects::all() } else { f.effects };
        if is_test {
            effects.fail = true;
        }
        let ret = if is_main { None } else { f.ret };
        if sigs.contains_key(&f.name) {
            return Err(ce(f.line, format!("function `{}` is defined twice", f.name)));
        }
        sigs.insert(f.name.clone(), Sig { params, ret, effects, is_main });
    }
    Ok(sigs)
}

fn c_params(sig: &Sig) -> String {
    let ps: Vec<String> = sig
        .params
        .iter()
        .filter(|(_, t)| is_value(*t))
        .map(|(n, t)| format!("{} {}", cty(*t), n))
        .collect();
    if ps.is_empty() { "void".into() } else { ps.join(", ") }
}

fn c_ret(sig: &Sig) -> &'static str {
    if sig.is_main {
        "int"
    } else {
        match sig.ret {
            None => "void",
            Some(t) => cty(t),
        }
    }
}

fn fail_ret(sig: &Sig) -> &'static str {
    if sig.is_main {
        return "return 1;";
    }
    match sig.ret {
        None => "return;",
        Some(Ty::Str) => "return \"\";",
        Some(Ty::Int) | Some(Ty::Bool) => "return 0;",
        // A record return on the failure path: a zero-initialised struct. The
        // caller checks the flag, never the value, so the contents are unused.
        Some(Ty::User(_)) => "return (typeof_ret){0};",
        Some(_) => "return;",
    }
}

/// Split items into the functions and record types, set the global type table.
fn prepare(items: &[Item]) -> (Vec<&Func>, Vec<&RecordDef>) {
    let funcs: Vec<&Func> = items.iter().filter_map(|i| match i {
        Item::Func(f) => Some(f),
        _ => None,
    }).collect();
    let records: Vec<&RecordDef> = items.iter().filter_map(|i| match i {
        Item::Type(r) => Some(r),
        _ => None,
    }).collect();
    let owned: Vec<RecordDef> = records.iter().map(|r| (*r).clone()).collect();
    let _ = TYPES.set(owned);
    (funcs, records)
}

fn emit_typedefs(records: &[&RecordDef], out: &mut String) {
    for r in records {
        out.push_str("typedef struct { ");
        for (fname, ft) in &r.fields {
            out.push_str(&format!("{} {}; ", cty(*ft), fname));
        }
        out.push_str(&format!("}} {};\n", r.name));
    }
    if !records.is_empty() {
        out.push('\n');
    }
}

fn emit(items: &[Item]) -> Result<String, CErr> {
    let (funcs, records) = prepare(items);
    let sigs = build_sigs(&funcs)?;
    if !sigs.contains_key("main") {
        return Err(ce(1, "no `main` function found"));
    }
    let mut out = String::from(RUNTIME);
    out.push('\n');
    emit_typedefs(&records, &mut out);
    for f in &funcs {
        if f.name != "main" {
            let sig = &sigs[&f.name];
            out.push_str(&format!("{} {}({});\n", c_ret(sig), f.name, c_params(sig)));
        }
    }
    out.push('\n');
    for f in &funcs {
        if f.name != "main" {
            out.push_str(&emit_func(f, &sigs)?);
        }
    }
    let mainf = funcs.iter().find(|f| f.name == "main").unwrap();
    out.push_str(&emit_func(mainf, &sigs)?);
    Ok(out)
}

/// Emit a test-runner program: every `test_*` function is called, its pass or
/// fail reported via the failure flag. No user `main` is needed or used.
fn emit_tests(items: &[Item]) -> Result<String, CErr> {
    let (funcs, records) = prepare(items);
    let sigs = build_sigs(&funcs)?;
    let mut out = String::from(RUNTIME);
    out.push('\n');
    emit_typedefs(&records, &mut out);
    for f in &funcs {
        if f.name != "main" {
            let sig = &sigs[&f.name];
            out.push_str(&format!("{} {}({});\n", c_ret(sig), f.name, c_params(sig)));
        }
    }
    out.push('\n');
    for f in &funcs {
        if f.name != "main" {
            out.push_str(&emit_func(f, &sigs)?);
        }
    }
    let mut tests = Vec::new();
    for f in &funcs {
        if f.name.starts_with("test_") {
            if !f.params.is_empty() {
                return Err(ce(f.line, format!("test `{}` must take no parameters", f.name)));
            }
            tests.push(f.name.clone());
        }
    }
    out.push_str("int main(void) {\n    int failures = 0;\n");
    for t in &tests {
        out.push_str(&format!("    simpler_failed = 0;\n    {}();\n", t));
        out.push_str(&format!("    if (simpler_failed) {{ printf(\"FAIL {}\\n\"); failures++; }}\n", t));
        out.push_str(&format!("    else printf(\"PASS {}\\n\");\n", t));
    }
    out.push_str(&format!("    printf(\"\\n%d/{} passed\\n\", {} - failures);\n", tests.len(), tests.len()));
    out.push_str("    return failures ? 1 : 0;\n}\n");
    Ok(out)
}

fn emit_func(f: &Func, sigs: &HashMap<String, Sig>) -> Result<String, CErr> {
    let sig = &sigs[&f.name];
    let fret = fail_ret(sig);
    let mut scope = Scope::new();
    for (pn, pt) in &sig.params {
        scope.declare(pn.clone(), *pt);
    }
    let mut used = Effects::default();
    let mut body = String::new();
    let n = f.body.len();
    if sig.ret.is_some() && n == 0 {
        return Err(ce(f.line, format!("`{}` must return a {:?} but its body is empty", f.name, sig.ret.unwrap())));
    }
    for (i, st) in f.body.iter().enumerate() {
        let is_return = sig.ret.is_some() && i + 1 == n;
        if is_return {
            emit_return(st, sig.ret.unwrap(), &mut scope, &mut body, sigs, &mut used, fret)?;
        } else {
            emit_stmt(st, &mut scope, &mut body, 1, sigs, &mut used, fret)?;
        }
    }
    if !used.covered_by(sig.effects) {
        let mut missing = Vec::new();
        if used.io && !sig.effects.io { missing.push("!IO"); }
        if used.fail && !sig.effects.fail { missing.push("!Fail"); }
        let m = missing.join(" ");
        return Err(ce(f.line, format!("`{}` uses {} but isn't declared {}", f.name, m, m)));
    }
    // A record's failure-path return needs the concrete struct name.
    let body = body.replace("typeof_ret", c_ret(sig));
    let header = if sig.is_main {
        "int main(void) {\n".to_string()
    } else {
        format!("{} {}({}) {{\n", c_ret(sig), f.name, c_params(sig))
    };
    let tail = if sig.is_main { "    return 0;\n}\n\n" } else { "}\n\n" };
    Ok(format!("{}{}{}", header, body, tail))
}

fn emit_return(
    st: &Stmt,
    ret_ty: Ty,
    scope: &mut Scope,
    out: &mut String,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
    fret: &str,
) -> Result<(), CErr> {
    let line = st.line;
    let e = match &st.kind {
        SKind::Expr(e) => e,
        _ => return Err(ce(line, format!("the last statement must produce the {:?} to return", ret_ty))),
    };
    if let Expr::Try(inner) = e {
        let (cval, vt, eff) = emit_failable(inner, scope, line, sigs, used)?;
        used.union(eff);
        if vt != ret_ty {
            return Err(ce(line, format!("returns {:?} but should return {:?}", vt, ret_ty)));
        }
        out.push_str(&format!("    {} _r = {};\n", cty(ret_ty), cval));
        out.push_str(&format!("    if (simpler_failed) {{ {} }}\n", fret));
        out.push_str("    return _r;\n");
    } else {
        let (cexpr, ty) = emit_expr(e, scope, line, sigs, used)?;
        if ty != ret_ty {
            return Err(ce(line, format!("returns {:?} but should return {:?}", ty, ret_ty)));
        }
        out.push_str(&format!("    return {};\n", cexpr));
    }
    Ok(())
}

fn emit_stmt(
    s: &Stmt,
    scope: &mut Scope,
    out: &mut String,
    ind: usize,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
    fret: &str,
) -> Result<(), CErr> {
    let pad = "    ".repeat(ind);
    let line = s.line;
    match &s.kind {
        SKind::Bind { name, ty, value: Expr::Try(inner) } => {
            let (cval, vt, eff) = emit_failable(inner, scope, line, sigs, used)?;
            used.union(eff);
            if let Some(a) = ty {
                if *a != vt {
                    return Err(ce(line, format!("`{}` declared {:?} but value is {:?}", name, a, vt)));
                }
            }
            bind(name, vt, &cval, scope, out, &pad, line)?;
            out.push_str(&format!("{}if (simpler_failed) {{ {} }}\n", pad, fret));
        }
        SKind::Bind { name, ty, value } => {
            let (vc, vt) = emit_expr(value, scope, line, sigs, used)?;
            if !is_value(vt) {
                return Err(ce(line, format!("cannot bind a {:?} to a name (capabilities are parameters only)", vt)));
            }
            if let Some(a) = ty {
                if *a != vt {
                    return Err(ce(line, format!("`{}` declared {:?} but value is {:?}", name, a, vt)));
                }
            }
            bind(name, vt, &vc, scope, out, &pad, line)?;
        }
        SKind::If { cond, then, els } => {
            let (cc, ct) = emit_expr(cond, scope, line, sigs, used)?;
            if ct != Ty::Bool {
                return Err(ce(line, "`if` condition must be Bool"));
            }
            out.push_str(&format!("{}if ({}) {{\n", pad, cc));
            scope.push();
            for s in then {
                emit_stmt(s, scope, out, ind + 1, sigs, used, fret)?;
            }
            scope.pop();
            out.push_str(&format!("{}}}", pad));
            if !els.is_empty() {
                out.push_str(" else {\n");
                scope.push();
                for s in els {
                    emit_stmt(s, scope, out, ind + 1, sigs, used, fret)?;
                }
                scope.pop();
                out.push_str(&format!("{}}}", pad));
            }
            out.push('\n');
        }
        SKind::Expr(e) => emit_expr_stmt(e, scope, out, ind, &pad, line, sigs, used, fret)?,
    }
    Ok(())
}

fn bind(name: &str, vt: Ty, cval: &str, scope: &mut Scope, out: &mut String, pad: &str, line: u32) -> Result<(), CErr> {
    match scope.lookup(name) {
        Some(prev) if prev != vt => {
            Err(ce(line, format!("`{}` is {:?}, cannot reassign a {:?}", name, prev, vt)))
        }
        Some(_) => {
            out.push_str(&format!("{}{} = {};\n", pad, name, cval));
            Ok(())
        }
        None => {
            scope.declare(name.to_string(), vt);
            out.push_str(&format!("{}{} {} = {};\n", pad, cty(vt), name, cval));
            Ok(())
        }
    }
}

fn emit_expr_stmt(
    e: &Expr,
    scope: &mut Scope,
    out: &mut String,
    ind: usize,
    pad: &str,
    line: u32,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
    fret: &str,
) -> Result<(), CErr> {
    match e {
        Expr::Send { recv, name, args, parens: false, block: Some(blk) } if name == "times" && args.is_empty() => {
            let (rc, rt) = emit_expr(recv, scope, line, sigs, used)?;
            if rt != Ty::Int {
                return Err(ce(line, "`times` expects an Int receiver"));
            }
            let var = blk.param.clone().unwrap_or_else(|| "_i".into());
            out.push_str(&format!("{}for (long {} = 0; {} < ({}); {}++) {{\n", pad, var, var, rc, var));
            scope.push();
            scope.declare(var.clone(), Ty::Int);
            for s in &blk.body {
                emit_stmt(s, scope, out, ind + 1, sigs, used, fret)?;
            }
            scope.pop();
            out.push_str(&format!("{}}}\n", pad));
            Ok(())
        }
        Expr::Send { recv, name, args, parens: true, block: None } => {
            let (_, rty) = emit_expr(recv, scope, line, sigs, used)?;
            match (rty, name.as_str()) {
                (Ty::Screen, "print") => {
                    let a = one_positional(args, line, "print")?;
                    let (ac, at) = emit_expr(a, scope, line, sigs, used)?;
                    let call = match at {
                        Ty::Str => format!("simpler_print_str({});", ac),
                        Ty::Int => format!("simpler_print_int({});", ac),
                        Ty::Bool => format!("simpler_print_bool({});", ac),
                        _ => return Err(ce(line, format!("cannot print a {:?}", at))),
                    };
                    used.io = true;
                    out.push_str(&format!("{}{}\n", pad, call));
                    Ok(())
                }
                (Ty::Mail, "send") => {
                    let (mut to, mut subject, mut body) = (None, None, None);
                    for a in args {
                        let n = a.name.as_deref().ok_or_else(|| {
                            ce(line, "`send` arguments must be named, e.g. to = \"...\", body = \"...\"")
                        })?;
                        let (vc, vt) = emit_expr(&a.value, scope, line, sigs, used)?;
                        if vt != Ty::Str {
                            return Err(ce(line, format!("`{}` must be a Str", n)));
                        }
                        match n {
                            "to" => to = Some(vc),
                            "subject" => subject = Some(vc),
                            "body" => body = Some(vc),
                            _ => return Err(ce(line, format!("`send` has no argument `{}`", n))),
                        }
                    }
                    let to = to.ok_or_else(|| ce(line, "`send` needs `to`"))?;
                    let body = body.ok_or_else(|| ce(line, "`send` needs `body`"))?;
                    let subject = subject.unwrap_or_else(|| "\"\"".into());
                    used.io = true;
                    out.push_str(&format!("{}simpler_send({}, {}, {});\n", pad, to, subject, body));
                    Ok(())
                }
                (Ty::Files, "read") => Err(ce(line, "`files.read` can fail; bind it as `name = files.read(path)?`")),
                _ => Err(ce(line, format!("{:?} has no method `{}`", rty, name))),
            }
        }
        Expr::Call { name, args } if name == "assert" => {
            let a = one_positional(args, line, "assert")?;
            let (ac, at) = emit_expr(a, scope, line, sigs, used)?;
            if at != Ty::Bool {
                return Err(ce(line, format!("`assert` needs a Bool, got {:?}", at)));
            }
            used.fail = true;
            out.push_str(&format!("{}if (!({})) {{ simpler_failed = 1; {} }}\n", pad, ac, fret));
            Ok(())
        }
        Expr::Call { name, .. } if find_record(name).is_some() => {
            Err(ce(line, format!("record `{}` must be bound to a name, not used as a statement", name)))
        }
        Expr::Call { name, args } => {
            let sig = sigs.get(name).ok_or_else(|| ce(line, format!("unknown function `{}`", name)))?;
            if sig.effects.fail {
                return Err(ce(line, format!("`{}` can fail; use `{}(...)?`", name, name)));
            }
            let cargs = call_args(name, args, sig, scope, line, sigs, used)?;
            used.union(sig.effects);
            out.push_str(&format!("{}{}({});\n", pad, name, cargs.join(", ")));
            Ok(())
        }
        Expr::Try(_) => Err(ce(line, "`?` must be the whole right-hand side of a binding, or the returned value")),
        _ => Err(ce(line, "this expression can't stand alone as a statement")),
    }
}

fn one_positional<'a>(args: &'a [Arg], line: u32, method: &str) -> Result<&'a Expr, CErr> {
    if args.len() != 1 || args[0].name.is_some() {
        return Err(ce(line, format!("`{}` takes one positional argument", method)));
    }
    Ok(&args[0].value)
}

fn call_args(
    name: &str,
    args: &[Arg],
    sig: &Sig,
    scope: &Scope,
    line: u32,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
) -> Result<Vec<String>, CErr> {
    if args.len() != sig.params.len() {
        return Err(ce(line, format!("`{}` takes {} argument(s), got {}", name, sig.params.len(), args.len())));
    }
    let mut cargs = Vec::new();
    for (a, (pn, pt)) in args.iter().zip(&sig.params) {
        if a.name.is_some() {
            return Err(ce(line, format!("`{}` takes positional arguments", name)));
        }
        let (ac, at) = emit_expr(&a.value, scope, line, sigs, used)?;
        if at != *pt {
            return Err(ce(line, format!("argument `{}` to `{}` expects {:?}, got {:?}", pn, name, pt, at)));
        }
        if is_value(*pt) {
            cargs.push(ac);
        }
    }
    Ok(cargs)
}

/// Lower a failable call (the inside of a `?`): `files.read(path)` or a user
/// function declared `!Fail`. Returns its C text, success type, and effects.
fn emit_failable(
    inner: &Expr,
    scope: &Scope,
    line: u32,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
) -> Result<(String, Ty, Effects), CErr> {
    match inner {
        Expr::Send { recv, name, args, parens: true, block: None } if name == "read" => {
            let (_, rty) = emit_expr(recv, scope, line, sigs, used)?;
            if rty != Ty::Files {
                return Err(ce(line, format!("`read` is a method of Files, not {:?}", rty)));
            }
            let a = one_positional(args, line, "read")?;
            let (ac, at) = emit_expr(a, scope, line, sigs, used)?;
            if at != Ty::Str {
                return Err(ce(line, "`read` expects a Str path"));
            }
            Ok((format!("simpler_read({})", ac), Ty::Str, Effects::io_fail()))
        }
        Expr::Call { name, args } => {
            let sig = sigs.get(name).ok_or_else(|| ce(line, format!("unknown function `{}`", name)))?;
            let ret = sig.ret.ok_or_else(|| ce(line, format!("`{}` returns nothing; `?` needs a value", name)))?;
            if !sig.effects.fail {
                return Err(ce(line, format!("`{}` cannot fail; the `?` is not needed", name)));
            }
            let cargs = call_args(name, args, sig, scope, line, sigs, used)?;
            Ok((format!("{}({})", name, cargs.join(", ")), ret, sig.effects))
        }
        _ => Err(ce(line, "`?` expects a failable call like `files.read(path)` or `f(...)`")),
    }
}

/// Emit a value or capability expression, returning its C text and type.
fn emit_expr(
    e: &Expr,
    scope: &Scope,
    line: u32,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
) -> Result<(String, Ty), CErr> {
    match e {
        Expr::Int(n) => Ok((n.to_string(), Ty::Int)),
        Expr::Str(s) => Ok((format!("\"{}\"", c_escape(s)), Ty::Str)),
        Expr::Var(n) => scope
            .lookup(n)
            .map(|t| (n.clone(), t))
            .ok_or_else(|| ce(line, format!("unknown name `{}`", n))),
        Expr::Bin { op, lhs, rhs } => {
            let (lc, lt) = emit_expr(lhs, scope, line, sigs, used)?;
            let (rc, rt) = emit_expr(rhs, scope, line, sigs, used)?;
            if lt != Ty::Int || rt != Ty::Int {
                return Err(ce(line, format!("operator `{}` needs Int operands", op_sym(*op))));
            }
            let (cop, res) = match op {
                BinOp::Add => ("+", Ty::Int),
                BinOp::Sub => ("-", Ty::Int),
                BinOp::Mul => ("*", Ty::Int),
                BinOp::Div => ("/", Ty::Int),
                BinOp::Eq => ("==", Ty::Bool),
                BinOp::Lt => ("<", Ty::Bool),
                BinOp::Gt => (">", Ty::Bool),
            };
            Ok((format!("({} {} {})", lc, cop, rc), res))
        }
        Expr::Call { name, .. } if name == "assert" => {
            Err(ce(line, "`assert` is a statement, not a value"))
        }
        // record construction: `Point(x = 3, y = 4)`
        Expr::Call { name, args } if find_record(name).is_some() => {
            let rec = find_record(name).unwrap();
            let mut provided: HashMap<&str, String> = HashMap::new();
            for a in args {
                let fname = a.name.as_deref().ok_or_else(|| {
                    ce(line, format!("`{}` fields must be named, e.g. {} = ...", name, rec.fields[0].0))
                })?;
                let field = rec.fields.iter().find(|(f, _)| f == fname).ok_or_else(|| {
                    ce(line, format!("`{}` has no field `{}`", name, fname))
                })?;
                let (vc, vt) = emit_expr(&a.value, scope, line, sigs, used)?;
                if vt != field.1 {
                    return Err(ce(line, format!("field `{}` of `{}` is {:?}, got {:?}", fname, name, field.1, vt)));
                }
                if provided.insert(field.0.as_str(), vc).is_some() {
                    return Err(ce(line, format!("field `{}` of `{}` set twice", fname, name)));
                }
            }
            let mut parts = Vec::new();
            for (fname, _) in &rec.fields {
                let v = provided.get(fname.as_str()).ok_or_else(|| {
                    ce(line, format!("missing field `{}` of `{}`", fname, name))
                })?;
                parts.push(format!(".{} = ({})", fname, v));
            }
            Ok((format!("({}){{ {} }}", rec.name, parts.join(", ")), Ty::User(rec.name)))
        }
        Expr::Call { name, args } => {
            let sig = sigs.get(name).ok_or_else(|| ce(line, format!("unknown function `{}`", name)))?;
            let ret = sig
                .ret
                .ok_or_else(|| ce(line, format!("`{}` returns nothing; it can't be used as a value", name)))?;
            if sig.effects.fail {
                return Err(ce(line, format!("`{}` can fail; use `{}(...)?`", name, name)));
            }
            let cargs = call_args(name, args, sig, scope, line, sigs, used)?;
            used.union(sig.effects);
            Ok((format!("{}({})", name, cargs.join(", ")), ret))
        }
        // capability member (`sys.screen`) or record field (`p.x`)
        Expr::Send { recv, name, args, parens: false, block: None } if args.is_empty() => {
            let (rc, rty) = emit_expr(recv, scope, line, sigs, used)?;
            if let Some(t) = cap_member(rty, name) {
                return Ok((String::new(), t));
            }
            if let Ty::User(tn) = rty {
                if let Some(rec) = find_record(tn) {
                    if let Some((_, ft)) = rec.fields.iter().find(|(f, _)| f == name) {
                        return Ok((format!("({}).{}", rc, name), *ft));
                    }
                }
                return Err(ce(line, format!("`{}` has no field `{}`", tn, name)));
            }
            Err(ce(line, format!("{:?} has no member `{}`", rty, name)))
        }
        Expr::Send { name, .. } if name == "read" => {
            Err(ce(line, "`files.read` can fail; bind it as `name = files.read(path)?`"))
        }
        Expr::Send { .. } => Err(ce(line, "this send does not produce a value here")),
        Expr::Try(_) => Err(ce(line, "`?` must be the whole right-hand side of a binding, or the returned value")),
    }
}

fn c_escape(s: &str) -> String {
    let mut o = String::new();
    for c in s.chars() {
        match c {
            '"' => o.push_str("\\\""),
            '\\' => o.push_str("\\\\"),
            '\n' => o.push_str("\\n"),
            '\t' => o.push_str("\\t"),
            '\r' => o.push_str("\\r"),
            c => o.push(c),
        }
    }
    o
}

// ----------------------------- Formatter -----------------------------------

#[derive(Debug, Clone)]
struct Cmt {
    line: u32,
    trailing: bool,
    text: String,
}

fn ty_name(t: Ty) -> &'static str {
    match t {
        Ty::Int => "Int",
        Ty::Str => "Str",
        Ty::Bool => "Bool",
        Ty::Sys => "Sys",
        Ty::Screen => "Screen",
        Ty::Files => "Files",
        Ty::Mail => "Mail",
        Ty::User(s) => s,
    }
}

fn effects_str(e: Effects) -> String {
    let mut s = String::new();
    if e.io { s.push_str(" !IO"); }
    if e.fail { s.push_str(" !Fail"); }
    s
}

fn prec(op: BinOp) -> u8 {
    match op {
        BinOp::Mul | BinOp::Div => 2,
        BinOp::Add | BinOp::Sub => 1,
        BinOp::Eq | BinOp::Lt | BinOp::Gt => 0,
    }
}

struct Cur<'a> {
    cs: &'a [Cmt],
    i: usize,
}

impl<'a> Cur<'a> {
    fn new(cs: &'a [Cmt]) -> Self { Cur { cs, i: 0 } }

    fn leading(&mut self, before: u32, ind: usize, out: &mut String) {
        while self.i < self.cs.len() && self.cs[self.i].line < before {
            out.push_str(&"  ".repeat(ind));
            out.push_str(&format!("// {}\n", self.cs[self.i].text));
            self.i += 1;
        }
    }

    fn trailing(&mut self, line: u32) -> Option<String> {
        if self.i < self.cs.len() && self.cs[self.i].line == line && self.cs[self.i].trailing {
            let t = self.cs[self.i].text.clone();
            self.i += 1;
            Some(t)
        } else {
            None
        }
    }

    fn rest(&mut self, out: &mut String) {
        while self.i < self.cs.len() {
            out.push_str(&format!("// {}\n", self.cs[self.i].text));
            self.i += 1;
        }
    }
}

fn fmt_program(items: &[Item], comments: &[Cmt]) -> String {
    let mut out = String::new();
    let mut cur = Cur::new(comments);
    for (idx, item) in items.iter().enumerate() {
        let line = match item {
            Item::Func(f) => f.line,
            Item::Type(r) => r.line,
        };
        if idx > 0 {
            out.push('\n');
        }
        cur.leading(line, 0, &mut out);
        match item {
            Item::Func(f) => format_fn(f, &mut cur, &mut out),
            Item::Type(r) => format_typedef(r, &mut cur, &mut out),
        }
    }
    cur.rest(&mut out);
    out
}

fn format_typedef(r: &RecordDef, cur: &mut Cur, out: &mut String) {
    out.push_str(&format!("{} = type {{", r.name));
    end_line(r.line, cur, out);
    for (fname, ft) in &r.fields {
        out.push_str(&format!("  {} : {}\n", fname, ty_name(*ft)));
    }
    out.push_str("}\n");
}

fn format_fn(f: &Func, cur: &mut Cur, out: &mut String) {
    let params = f
        .params
        .iter()
        .map(|(n, t)| match t {
            Some(ty) => format!("{} : {}", n, ty_name(*ty)),
            None => n.clone(),
        })
        .collect::<Vec<_>>()
        .join(", ");
    let ret = match f.ret {
        Some(t) => format!(" : {}", ty_name(t)),
        None => String::new(),
    };
    out.push_str(&format!("{}({}){}{} {{", f.name, params, ret, effects_str(f.effects)));
    end_line(f.line, cur, out);
    for st in &f.body {
        format_stmt(st, 1, cur, out);
    }
    out.push_str("}\n");
}

fn format_stmt(st: &Stmt, ind: usize, cur: &mut Cur, out: &mut String) {
    cur.leading(st.line, ind, out);
    let pad = "  ".repeat(ind);
    match &st.kind {
        SKind::Bind { name, ty, value } => {
            match ty {
                Some(t) => out.push_str(&format!("{}{} : {} = {}", pad, name, ty_name(*t), fmt_expr(value))),
                None => out.push_str(&format!("{}{} = {}", pad, name, fmt_expr(value))),
            }
            end_line(st.line, cur, out);
        }
        SKind::If { cond, then, els } => {
            out.push_str(&format!("{}if {} {{", pad, fmt_expr(cond)));
            end_line(st.line, cur, out);
            for s in then {
                format_stmt(s, ind + 1, cur, out);
            }
            if els.is_empty() {
                out.push_str(&format!("{}}}\n", pad));
            } else {
                out.push_str(&format!("{}}} else {{\n", pad));
                for s in els {
                    format_stmt(s, ind + 1, cur, out);
                }
                out.push_str(&format!("{}}}\n", pad));
            }
        }
        SKind::Expr(Expr::Send { recv, name, block: Some(blk), .. }) if name == "times" => {
            let p = blk.param.clone().unwrap_or_else(|| "_".into());
            out.push_str(&format!("{}{}.times {{ {} in", pad, fmt_expr(recv), p));
            end_line(st.line, cur, out);
            for s in &blk.body {
                format_stmt(s, ind + 1, cur, out);
            }
            out.push_str(&format!("{}}}\n", pad));
        }
        SKind::Expr(e) => {
            out.push_str(&format!("{}{}", pad, fmt_expr(e)));
            end_line(st.line, cur, out);
        }
    }
}

fn end_line(line: u32, cur: &mut Cur, out: &mut String) {
    if let Some(tc) = cur.trailing(line) {
        out.push_str(&format!("  // {}", tc));
    }
    out.push('\n');
}

fn fmt_expr(e: &Expr) -> String {
    fmt_prec(e, 0)
}

fn fmt_prec(e: &Expr, min: u8) -> String {
    match e {
        Expr::Int(n) => n.to_string(),
        Expr::Str(s) => format!("\"{}\"", c_escape(s)),
        Expr::Var(n) => n.clone(),
        Expr::Bin { op, lhs, rhs } => {
            let p = prec(*op);
            let s = format!("{} {} {}", fmt_prec(lhs, p), op_sym(*op), fmt_prec(rhs, p + 1));
            if p < min { format!("({})", s) } else { s }
        }
        Expr::Call { name, args } => format!("{}({})", name, fmt_args(args)),
        Expr::Send { recv, name, args, parens, block } => {
            let r = fmt_prec(recv, 100);
            if block.is_some() {
                format!("{}.{}", r, name)
            } else if *parens {
                format!("{}.{}({})", r, name, fmt_args(args))
            } else {
                format!("{}.{}", r, name)
            }
        }
        Expr::Try(inner) => format!("{}?", fmt_prec(inner, 100)),
    }
}

fn fmt_args(args: &[Arg]) -> String {
    args.iter()
        .map(|a| match &a.name {
            Some(n) => format!("{} = {}", n, fmt_expr(&a.value)),
            None => fmt_expr(&a.value),
        })
        .collect::<Vec<_>>()
        .join(", ")
}

// ----------------------------- Driver / CLI --------------------------------

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        usage();
        exit(0);
    }
    match args[1].as_str() {
        cmd @ ("build" | "run" | "emit") => {
            let Some(file) = args.get(2) else {
                eprintln!("simpler: `{}` needs an input file", cmd);
                exit(2);
            };
            if let Err(e) = drive(cmd, file) {
                eprintln!("simpler: {}", e);
                exit(1);
            }
        }
        "fmt" => {
            let Some(file) = args.get(2) else {
                eprintln!("simpler: `fmt` needs an input file");
                exit(2);
            };
            if let Err(e) = fmt_file(file) {
                eprintln!("simpler: {}", e);
                exit(1);
            }
        }
        "test" => {
            let Some(file) = args.get(2) else {
                eprintln!("simpler: `test` needs an input file");
                exit(2);
            };
            if let Err(e) = test_file(file) {
                eprintln!("simpler: {}", e);
                exit(1);
            }
        }
        "-h" | "--help" | "help" => usage(),
        other => {
            eprintln!("simpler: unknown command `{}`\n", other);
            usage();
            exit(2);
        }
    }
}

fn usage() {
    eprintln!(
        "simpler — the Simpler language (bootstrap)\n\n\
         usage:\n\
         \x20 simpler run   <file.smplr>   build and run\n\
         \x20 simpler build <file.smplr>   transpile to C and compile\n\
         \x20 simpler emit  <file.smplr>   print the generated C\n\
         \x20 simpler fmt   <file.smplr>   format in place (canonical)\n\
         \x20 simpler test  <file.smplr>   run the test_* functions"
    );
}

fn drive(cmd: &str, file: &str) -> Result<(), String> {
    let src = fs::read_to_string(file).map_err(|e| format!("cannot read {}: {}", file, e))?;
    let (toks, _comments) = lex(&src).map_err(|e| diag(file, &src, &e))?;
    let mut p = Parser::new(toks);
    let items = p.program().map_err(|e| diag(file, &src, &e))?;
    let c = emit(&items).map_err(|e| diag(file, &src, &e))?;

    if cmd == "emit" {
        print!("{}", c);
        return Ok(());
    }
    compile_c(file, &c, cmd == "run")
}

fn test_file(file: &str) -> Result<(), String> {
    let src = fs::read_to_string(file).map_err(|e| format!("cannot read {}: {}", file, e))?;
    let (toks, _comments) = lex(&src).map_err(|e| diag(file, &src, &e))?;
    let mut p = Parser::new(toks);
    let items = p.program().map_err(|e| diag(file, &src, &e))?;
    let c = emit_tests(&items).map_err(|e| diag(file, &src, &e))?;
    compile_c(file, &c, true)
}

fn fmt_file(file: &str) -> Result<(), String> {
    let src = fs::read_to_string(file).map_err(|e| format!("cannot read {}: {}", file, e))?;
    let (toks, comments) = lex(&src).map_err(|e| diag(file, &src, &e))?;
    let mut p = Parser::new(toks);
    let items = p.program().map_err(|e| diag(file, &src, &e))?;
    let formatted = fmt_program(&items, &comments);
    fs::write(file, &formatted).map_err(|e| format!("cannot write {}: {}", file, e))?;
    println!("formatted {}", file);
    Ok(())
}

fn compile_c(file: &str, c: &str, run: bool) -> Result<(), String> {
    let path = Path::new(file);
    let stem = path
        .file_stem()
        .and_then(|s| s.to_str())
        .ok_or("bad input file name")?;
    let dir = path
        .parent()
        .filter(|d| !d.as_os_str().is_empty())
        .unwrap_or(Path::new("."));
    let cfile = dir.join(format!("{}.c", stem));
    let bin = dir.join(stem);

    fs::write(&cfile, c).map_err(|e| format!("cannot write {}: {}", cfile.display(), e))?;

    let cc = pick_cc();
    let status = Command::new(&cc)
        .arg(&cfile)
        .arg("-o")
        .arg(&bin)
        .status()
        .map_err(|e| format!("failed to launch C compiler `{}`: {}", cc, e))?;
    if !status.success() {
        return Err("C compilation failed".into());
    }

    if run {
        let st = Command::new(&bin)
            .status()
            .map_err(|e| format!("failed to run {}: {}", bin.display(), e))?;
        if !st.success() {
            exit(st.code().unwrap_or(1));
        }
    }
    Ok(())
}

fn diag(file: &str, src: &str, e: &CErr) -> String {
    let mut s = format!("{}:{}: {}", file, e.line, e.msg);
    if let Some(text) = src.lines().nth(e.line.saturating_sub(1) as usize) {
        s.push_str(&format!("\n  {:>4} | {}", e.line, text));
    }
    s
}

fn pick_cc() -> String {
    if let Ok(c) = env::var("CC") {
        if !c.is_empty() {
            return c;
        }
    }
    if Path::new("/usr/bin/cc").exists() {
        return "/usr/bin/cc".into();
    }
    "cc".into()
}
