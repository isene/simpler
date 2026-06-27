//! The Simpler bootstrap compiler — milestone M3.
//!
//! Pipeline: lex -> parse -> emit C -> system `cc`. M3 makes the language's
//! central idea real: effects live in the signature, and a function can only
//! touch the capabilities it was handed.
//!
//!   * effect annotations `!IO` / `!Fail` on functions (coarse vocabulary),
//!   * user functions with typed parameters and free calls,
//!   * capabilities as passed values (`Sys`, `Screen`), checked by type,
//!   * an effect checker: a function that uses an effect must declare it.
//!
//! `main` implicitly holds the world (all effects), so M1/M2 programs still
//! compile. Capabilities are erased at runtime — purely a compile-time
//! permission system, zero cost. `?`/`!Fail` propagation, `mail`/`files`,
//! and named arguments arrive in M3b.

use std::collections::HashMap;
use std::env;
use std::fs;
use std::path::Path;
use std::process::{exit, Command};

/// A compile error tied to a source line.
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
    Sys,    // the root capability (main's world)
    Screen, // the screen capability
}

/// Value types have a C representation; capabilities are erased at runtime.
fn is_value(t: Ty) -> bool {
    matches!(t, Ty::Int | Ty::Str | Ty::Bool)
}

fn cty(t: Ty) -> &'static str {
    match t {
        Ty::Int => "long",
        Ty::Str => "const char *",
        Ty::Bool => "int",
        Ty::Sys | Ty::Screen => unreachable!("capabilities have no C type"),
    }
}

/// A sub-capability or member reachable from a capability: `sys.screen`.
fn cap_member(recv: Ty, name: &str) -> Option<Ty> {
    match (recv, name) {
        (Ty::Sys, "screen") => Some(Ty::Screen),
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
    fn union(&mut self, o: Effects) {
        self.io |= o.io;
        self.fail |= o.fail;
    }
    /// True if every effect in `self` is also present in `decl`.
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
enum Expr {
    Int(i64),
    Str(String),
    Var(String),
    Bin { op: BinOp, lhs: Box<Expr>, rhs: Box<Expr> },
    /// Free function call: `name(args)`.
    Call { name: String, args: Vec<Expr> },
    /// Message send: `receiver.name(args)` or trailing-block `recv.name { ... }`.
    Send {
        recv: Box<Expr>,
        name: String,
        args: Vec<Expr>,
        parens: bool,
        block: Option<Block>,
    },
}

#[derive(Debug, Clone)]
struct Block {
    param: Option<String>,
    body: Vec<Stmt>,
}

/// A statement, tagged with the source line it starts on (for diagnostics).
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
    /// Declared parameters; the type is `None` only for `main`'s world.
    params: Vec<(String, Option<Ty>)>,
    effects: Effects,
    body: Vec<Stmt>,
    line: u32,
}

/// A resolved function signature, used to check calls and effects.
struct Sig {
    params: Vec<(String, Ty)>,
    effects: Effects,
    is_main: bool,
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
    Assign, // =
    EqEq,   // ==
    Colon,  // :
    Bang,   // !
    Plus,
    Minus,
    Star,
    Slash,
    Lt,
    Gt,
    Eof,
}

/// Tokenise into `(token, line)` pairs. Indexes a `Vec<char>` rather than
/// bytes, so multi-byte UTF-8 can never split mid-character.
fn lex(src: &str) -> Result<Vec<(Tok, u32)>, CErr> {
    let cs: Vec<char> = src.chars().collect();
    let mut i = 0;
    let mut line: u32 = 1;
    let mut toks: Vec<(Tok, u32)> = Vec::new();
    while i < cs.len() {
        let c = cs[i];
        match c {
            '\n' => { line += 1; i += 1; }
            ' ' | '\t' | '\r' => i += 1,
            '/' if i + 1 < cs.len() && cs[i + 1] == '/' => {
                while i < cs.len() && cs[i] != '\n' {
                    i += 1;
                }
            }
            '.' => { toks.push((Tok::Dot, line)); i += 1; }
            '(' => { toks.push((Tok::LParen, line)); i += 1; }
            ')' => { toks.push((Tok::RParen, line)); i += 1; }
            '{' => { toks.push((Tok::LBrace, line)); i += 1; }
            '}' => { toks.push((Tok::RBrace, line)); i += 1; }
            ',' => { toks.push((Tok::Comma, line)); i += 1; }
            '!' => { toks.push((Tok::Bang, line)); i += 1; }
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
    Ok(toks)
}

// ----------------------------- Parser --------------------------------------

struct Parser {
    toks: Vec<(Tok, u32)>,
    pos: usize,
    no_block: bool,
}

impl Parser {
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

    fn program(&mut self) -> Result<Vec<Func>, CErr> {
        let mut fns = Vec::new();
        while *self.peek() != Tok::Eof {
            fns.push(self.func()?);
        }
        Ok(fns)
    }

    /// func := ident '(' params? ')' ('!' effect)* '{' stmt* '}'
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
        Ok(Func { name, params, effects, body, line })
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
        // `name = value` or `name : Type = value` (but not `name(...)`, a call)
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
        self.pos += 1; // 'if'
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
            _ => Err(ce(line, format!("unknown type `{}`", n))),
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

    /// postfix := primary ('.' ident ('(' args ')' | '{' block '}')?)*
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
        Ok(e)
    }

    fn args(&mut self) -> Result<Vec<Expr>, CErr> {
        self.eat(&Tok::LParen)?;
        let mut args = Vec::new();
        if *self.peek() != Tok::RParen {
            loop {
                args.push(self.expr()?);
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

/* Generated by the Simpler bootstrap compiler. Do not edit. */

static void simpler_print_str(const char *s) { fputs(s, stdout); fputc('\\n', stdout); }
static void simpler_print_int(long n)        { printf(\"%ld\\n\", n); }
static void simpler_print_bool(int b)        { fputs(b ? \"true\" : \"false\", stdout); fputc('\\n', stdout); }
";

/// Resolve each function's signature: param types and declared effects.
/// `main` is special — its sole untyped parameter is the `Sys` world, and it
/// implicitly holds every effect.
fn build_sigs(funcs: &[Func]) -> Result<HashMap<String, Sig>, CErr> {
    let mut sigs = HashMap::new();
    for f in funcs {
        let is_main = f.name == "main";
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
        let effects = if is_main { Effects::all() } else { f.effects };
        if sigs.contains_key(&f.name) {
            return Err(ce(f.line, format!("function `{}` is defined twice", f.name)));
        }
        sigs.insert(f.name.clone(), Sig { params, effects, is_main });
    }
    if !sigs.contains_key("main") {
        return Err(ce(1, "no `main` function found"));
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

fn emit(funcs: &[Func]) -> Result<String, CErr> {
    let sigs = build_sigs(funcs)?;
    let mut out = String::from(RUNTIME);
    out.push('\n');
    // Forward prototypes for user functions, so call order never matters.
    for f in funcs {
        if f.name != "main" {
            out.push_str(&format!("void {}({});\n", f.name, c_params(&sigs[&f.name])));
        }
    }
    out.push('\n');
    // Bodies: helpers first, `main` last.
    for f in funcs {
        if f.name != "main" {
            out.push_str(&emit_func(f, &sigs)?);
        }
    }
    let mainf = funcs.iter().find(|f| f.name == "main").unwrap();
    out.push_str(&emit_func(mainf, &sigs)?);
    Ok(out)
}

fn emit_func(f: &Func, sigs: &HashMap<String, Sig>) -> Result<String, CErr> {
    let sig = &sigs[&f.name];
    let mut scope = Scope::new();
    for (pn, pt) in &sig.params {
        scope.declare(pn.clone(), *pt);
    }
    let mut used = Effects::default();
    let mut body = String::new();
    for st in &f.body {
        emit_stmt(st, &mut scope, &mut body, 1, sigs, &mut used)?;
    }
    // Effect check: every used effect must be declared (main holds all).
    if !used.covered_by(sig.effects) {
        let mut missing = Vec::new();
        if used.io && !sig.effects.io { missing.push("!IO"); }
        if used.fail && !sig.effects.fail { missing.push("!Fail"); }
        let m = missing.join(" ");
        return Err(ce(f.line, format!("`{}` uses {} but isn't declared {}", f.name, m, m)));
    }
    let header = if sig.is_main {
        "int main(void) {\n".to_string()
    } else {
        format!("void {}({}) {{\n", f.name, c_params(sig))
    };
    let tail = if sig.is_main { "    return 0;\n}\n\n" } else { "}\n\n" };
    Ok(format!("{}{}{}", header, body, tail))
}

fn emit_stmt(
    s: &Stmt,
    scope: &mut Scope,
    out: &mut String,
    ind: usize,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
) -> Result<(), CErr> {
    let pad = "    ".repeat(ind);
    let line = s.line;
    match &s.kind {
        SKind::Bind { name, ty, value } => {
            let (vc, vt) = emit_expr(value, scope, line)?;
            if !is_value(vt) {
                return Err(ce(line, format!("cannot bind a {:?} to a name (capabilities are parameters only)", vt)));
            }
            if let Some(a) = ty {
                if *a != vt {
                    return Err(ce(line, format!("`{}` declared {:?} but value is {:?}", name, a, vt)));
                }
            }
            match scope.lookup(name) {
                Some(prev) => {
                    if prev != vt {
                        return Err(ce(line, format!("`{}` is {:?}, cannot reassign a {:?}", name, prev, vt)));
                    }
                    out.push_str(&format!("{}{} = {};\n", pad, name, vc));
                }
                None => {
                    scope.declare(name.clone(), vt);
                    out.push_str(&format!("{}{} {} = {};\n", pad, cty(vt), name, vc));
                }
            }
        }
        SKind::If { cond, then, els } => {
            let (cc, ct) = emit_expr(cond, scope, line)?;
            if ct != Ty::Bool {
                return Err(ce(line, "`if` condition must be Bool"));
            }
            out.push_str(&format!("{}if ({}) {{\n", pad, cc));
            scope.push();
            for s in then {
                emit_stmt(s, scope, out, ind + 1, sigs, used)?;
            }
            scope.pop();
            out.push_str(&format!("{}}}", pad));
            if !els.is_empty() {
                out.push_str(" else {\n");
                scope.push();
                for s in els {
                    emit_stmt(s, scope, out, ind + 1, sigs, used)?;
                }
                scope.pop();
                out.push_str(&format!("{}}}", pad));
            }
            out.push('\n');
        }
        SKind::Expr(e) => emit_expr_stmt(e, scope, out, ind, &pad, line, sigs, used)?,
    }
    Ok(())
}

/// Statement-position expressions: capability sends (`print`), iteration
/// (`times`), and free function calls.
fn emit_expr_stmt(
    e: &Expr,
    scope: &mut Scope,
    out: &mut String,
    ind: usize,
    pad: &str,
    line: u32,
    sigs: &HashMap<String, Sig>,
    used: &mut Effects,
) -> Result<(), CErr> {
    match e {
        Expr::Send { recv, name, args, parens, block } => {
            match name.as_str() {
                // Screen.print(value) — !IO. Only a Screen capability has it.
                "print" if *parens && args.len() == 1 && block.is_none() => {
                    let (_, rty) = emit_expr(recv, scope, line)?;
                    if rty != Ty::Screen {
                        return Err(ce(line, format!("`print` is a method of Screen, not {:?}", rty)));
                    }
                    let (ac, at) = emit_expr(&args[0], scope, line)?;
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
                // n.times { i in ... } — iterate i over 0..n. No effect of its own.
                "times" if !*parens && args.is_empty() && block.is_some() => {
                    let (rc, rt) = emit_expr(recv, scope, line)?;
                    if rt != Ty::Int {
                        return Err(ce(line, "`times` expects an Int receiver"));
                    }
                    let blk = block.as_ref().unwrap();
                    let var = blk.param.clone().unwrap_or_else(|| "_i".into());
                    out.push_str(&format!("{}for (long {} = 0; {} < ({}); {}++) {{\n", pad, var, var, rc, var));
                    scope.push();
                    scope.declare(var.clone(), Ty::Int);
                    for s in &blk.body {
                        emit_stmt(s, scope, out, ind + 1, sigs, used)?;
                    }
                    scope.pop();
                    out.push_str(&format!("{}}}\n", pad));
                    Ok(())
                }
                _ => Err(ce(line, format!("unknown method `{}`", name))),
            }
        }
        Expr::Call { name, args } => {
            let sig = sigs
                .get(name)
                .ok_or_else(|| ce(line, format!("unknown function `{}`", name)))?;
            if args.len() != sig.params.len() {
                return Err(ce(line, format!("`{}` takes {} argument(s), got {}", name, sig.params.len(), args.len())));
            }
            let mut cargs = Vec::new();
            for (arg, (pn, pt)) in args.iter().zip(&sig.params) {
                let (ac, at) = emit_expr(arg, scope, line)?;
                if at != *pt {
                    return Err(ce(line, format!("argument `{}` to `{}` expects {:?}, got {:?}", pn, name, pt, at)));
                }
                if is_value(*pt) {
                    cargs.push(ac);
                }
            }
            used.union(sig.effects);
            out.push_str(&format!("{}{}({});\n", pad, name, cargs.join(", ")));
            Ok(())
        }
        _ => Err(ce(line, "this expression can't stand alone as a statement")),
    }
}

/// Emit a value or capability expression, returning its C text and its type.
/// Capability expressions have an empty C text (they are erased at runtime).
fn emit_expr(e: &Expr, scope: &Scope, line: u32) -> Result<(String, Ty), CErr> {
    match e {
        Expr::Int(n) => Ok((n.to_string(), Ty::Int)),
        Expr::Str(s) => Ok((format!("\"{}\"", c_escape(s)), Ty::Str)),
        Expr::Var(n) => scope
            .lookup(n)
            .map(|t| (n.clone(), t))
            .ok_or_else(|| ce(line, format!("unknown name `{}`", n))),
        Expr::Bin { op, lhs, rhs } => {
            let (lc, lt) = emit_expr(lhs, scope, line)?;
            let (rc, rt) = emit_expr(rhs, scope, line)?;
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
        // A capability member read, e.g. `sys.screen`.
        Expr::Send { recv, name, args, parens, block }
            if !*parens && args.is_empty() && block.is_none() =>
        {
            let (_, rty) = emit_expr(recv, scope, line)?;
            match cap_member(rty, name) {
                Some(t) => Ok((String::new(), t)),
                None => Err(ce(line, format!("{:?} has no member `{}`", rty, name))),
            }
        }
        Expr::Send { .. } => Err(ce(line, "this send does not produce a value")),
        Expr::Call { .. } => Err(ce(line, "functions don't return a value yet (M3b)")),
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
        cmd @ ("fmt" | "test") => {
            eprintln!("simpler: `{}` is not implemented yet (planned for M4)", cmd);
            exit(2);
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
         \x20 simpler fmt   <file.smplr>   (M4) not yet\n\
         \x20 simpler test  <file.smplr>   (M4) not yet"
    );
}

fn drive(cmd: &str, file: &str) -> Result<(), String> {
    let src = fs::read_to_string(file).map_err(|e| format!("cannot read {}: {}", file, e))?;
    let toks = lex(&src).map_err(|e| diag(file, &src, &e))?;
    let mut p = Parser { toks, pos: 0, no_block: false };
    let funcs = p.program().map_err(|e| diag(file, &src, &e))?;
    let c = emit(&funcs).map_err(|e| diag(file, &src, &e))?;

    if cmd == "emit" {
        print!("{}", c);
        return Ok(());
    }

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

    fs::write(&cfile, &c).map_err(|e| format!("cannot write {}: {}", cfile.display(), e))?;

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

    if cmd == "run" {
        let st = Command::new(&bin)
            .status()
            .map_err(|e| format!("failed to run {}: {}", bin.display(), e))?;
        if !st.success() {
            exit(st.code().unwrap_or(1));
        }
    }
    Ok(())
}

/// Format a compile error as `file:line: message` with the offending line.
fn diag(file: &str, src: &str, e: &CErr) -> String {
    let mut s = format!("{}:{}: {}", file, e.line, e.msg);
    if let Some(text) = src.lines().nth(e.line.saturating_sub(1) as usize) {
        s.push_str(&format!("\n  {:>4} | {}", e.line, text));
    }
    s
}

/// The system C compiler. On this machine `~/bin/cc` is a Claude session
/// launcher that shadows the real compiler, so prefer the absolute system
/// `cc` when present. `CC` overrides everything.
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
