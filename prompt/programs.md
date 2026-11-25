## Two whole programs

Both compile and pass their tests. Read them as examples of the language at size.

```
Ins = type { line : Int, op : Str, args : List[Str], dup : Str }

spec(op : Str) : Str {
  r = "?"
  if op == "MOV" || op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV" || op == "MOD" || op == "AND" || op == "OR" || op == "XOR" || op == "LOAD" {
    r = "RX"
  } else if op == "NEG" || op == "INC" || op == "DEC" || op == "POP" || op == "IN" {
    r = "R"
  } else if op == "CMP" || op == "STORE" {
    r = "XX"
  } else if op == "JMP" || op == "JEQ" || op == "JNE" || op == "JLT" || op == "JGT" || op == "JLE" || op == "JGE" || op == "CALL" {
    r = "L"
  } else if op == "PUSH" || op == "OUT" {
    r = "X"
  } else if op == "PRINT" {
    r = "S"
  } else if op == "RET" || op == "HALT" {
    r = ""
  }
  r
}

isName(s : Str) : Bool {
  if s.length == 0 {
    return false
  }
  f = s.at(0)
  if f >= "0" && f <= "9" {
    return false
  }
  for i in 0..s.length {
    c = s.at(i)
    ok = (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || (c >= "0" && c <= "9") || c == "_"
    if !ok {
      return false
    }
  }
  true
}

isReg(s : Str) : Bool {
  s.length == 2 && s.at(0) == "r" && s.at(1) >= "0" && s.at(1) <= "7"
}

err(ins : Ins, labels : Map) : Str {
  k = spec(ins.op)
  if k == "?" {
    return "unknown instruction " + ins.op
  }
  if ins.args.length != k.length {
    return ins.op + " takes " + k.length.toStr + " operands"
  }
  for i in 0..k.length {
    c = k.at(i)
    a = ins.args.at(i)
    bad = false
    if c == "R" && !isReg(a) {
      bad = true
    }
    if c == "X" && !(isReg(a) || a.isInt) {
      bad = true
    }
    if c == "L" && !isName(a) {
      bad = true
    }
    if c == "S" && !(a.length >= 2 && a.startsWith('"') && a.endsWith('"')) {
      bad = true
    }
    if bad {
      return "bad operand " + a
    }
  }
  for i in 0..k.length {
    if k.at(i) == "L" {
      a = ins.args.at(i)
      if !labels.has(a) {
        return "unknown label " + a
      }
    }
  }
  ""
}

val(x : Str, regs : List[Int]) : Int {
  if isReg(x) {
    return regs.at(x.slice(1).toInt)
  }
  x.toInt
}

fault(out : Screen, line : Int, msg : Str) !IO {
  out.print("error at line " + line.toStr + ": " + msg)
}

run(prog : List[Ins], labels : Map, nums : List[Int], out : Screen) !IO {
  regs : List[Int] = []
  for i in 0..8 {
    regs.push(0)
  }
  mem : List[Int] = []
  for i in 0..256 {
    mem.push(0)
  }
  stack : List[Int] = []
  flag = 0
  pc = 0
  steps = 0
  inpos = 0
  while pc < prog.length {
    ins = prog.at(pc)
    if steps >= 100000 {
      fault(out, ins.line, "step limit")
      return
    }
    steps = steps + 1
    op = ins.op
    a = ins.args
    npc = pc + 1
    if op == "HALT" {
      out.print("steps: " + steps.toStr)
      return
    } else if op == "MOV" {
      regs.set(a.at(0).slice(1).toInt, val(a.at(1), regs))
    } else if op == "ADD" || op == "SUB" || op == "MUL" || op == "AND" || op == "OR" || op == "XOR" {
      ri = a.at(0).slice(1).toInt
      x = regs.at(ri)
      y = val(a.at(1), regs)
      z = 0
      if op == "ADD" {
        z = x + y
      } else if op == "SUB" {
        z = x - y
      } else if op == "MUL" {
        z = x * y
      } else if op == "AND" {
        z = x.bitAnd(y)
      } else if op == "OR" {
        z = x.bitOr(y)
      } else {
        z = x.bitXor(y)
      }
      regs.set(ri, z)
    } else if op == "DIV" || op == "MOD" {
      ri = a.at(0).slice(1).toInt
      y = val(a.at(1), regs)
      if y == 0 {
        fault(out, ins.line, "division by zero")
        return
      }
      if op == "DIV" {
        regs.set(ri, regs.at(ri) / y)
      } else {
        regs.set(ri, regs.at(ri) % y)
      }
    } else if op == "NEG" {
      ri = a.at(0).slice(1).toInt
      regs.set(ri, 0 - regs.at(ri))
    } else if op == "INC" {
      ri = a.at(0).slice(1).toInt
      regs.set(ri, regs.at(ri) + 1)
    } else if op == "DEC" {
      ri = a.at(0).slice(1).toInt
      regs.set(ri, regs.at(ri) - 1)
    } else if op == "CMP" {
      x = val(a.at(0), regs)
      y = val(a.at(1), regs)
      if x < y {
        flag = -1
      } else if x == y {
        flag = 0
      } else {
        flag = 1
      }
    } else if op == "JMP" {
      npc = labels.get(a.at(0))
    } else if op == "JEQ" || op == "JNE" || op == "JLT" || op == "JGT" || op == "JLE" || op == "JGE" {
      go = false
      if op == "JEQ" {
        go = flag == 0
      } else if op == "JNE" {
        go = flag != 0
      } else if op == "JLT" {
        go = flag < 0
      } else if op == "JGT" {
        go = flag > 0
      } else if op == "JLE" {
        go = flag <= 0
      } else {
        go = flag >= 0
      }
      if go {
        npc = labels.get(a.at(0))
      }
    } else if op == "PUSH" {
      if stack.length >= 64 {
        fault(out, ins.line, "stack overflow")
        return
      }
      stack.push(val(a.at(0), regs))
    } else if op == "POP" {
      if stack.length == 0 {
        fault(out, ins.line, "stack underflow")
        return
      }
      regs.set(a.at(0).slice(1).toInt, stack.remove(stack.length - 1))
    } else if op == "CALL" {
      if stack.length >= 64 {
        fault(out, ins.line, "stack overflow")
        return
      }
      stack.push(pc + 1)
      npc = labels.get(a.at(0))
    } else if op == "RET" {
      if stack.length == 0 {
        fault(out, ins.line, "stack underflow")
        return
      }
      npc = stack.remove(stack.length - 1)
    } else if op == "LOAD" {
      addr = val(a.at(1), regs)
      if addr < 0 || addr > 255 {
        fault(out, ins.line, "bad address " + addr.toStr)
        return
      }
      regs.set(a.at(0).slice(1).toInt, mem.at(addr))
    } else if op == "STORE" {
      addr = val(a.at(0), regs)
      v = val(a.at(1), regs)
      if addr < 0 || addr > 255 {
        fault(out, ins.line, "bad address " + addr.toStr)
        return
      }
      mem.set(addr, v)
    } else if op == "IN" {
      if inpos >= nums.length {
        fault(out, ins.line, "no input")
        return
      }
      regs.set(a.at(0).slice(1).toInt, nums.at(inpos))
      inpos = inpos + 1
    } else if op == "OUT" {
      out.print(val(a.at(0), regs).toStr)
    } else if op == "PRINT" {
      s = a.at(0)
      out.print(s.slice(1, s.length - 1))
    }
    pc = npc
  }
  out.print("steps: " + steps.toStr)
}

main(sys) {
  lines = sys.stdin.split("\n")
  cut = lines.length
  for i in 0..lines.length {
    if cut == lines.length && lines.at(i).trim == "---" {
      cut = i
    }
  }
  labels = Map()
  prog : List[Ins] = []
  ents : List[Ins] = []
  for i in 0..cut {
    t = lines.at(i).trim
    if t.length > 0 && !t.startsWith(";") {
      lab = ""
      rest = t
      p = t.find(":")
      if p > 0 && isName(t.slice(0, p)) {
        lab = t.slice(0, p)
        rest = t.slice(p + 1).trim
      }
      dup = ""
      if lab != "" {
        if labels.has(lab) {
          dup = lab
        } else {
          labels.set(lab, prog.length)
        }
      }
      op = ""
      args : List[Str] = []
      if rest != "" {
        sp = rest.find(" ")
        txt = ""
        if sp < 0 {
          op = rest
        } else {
          op = rest.slice(0, sp)
          txt = rest.slice(sp + 1).trim
        }
        if txt != "" {
          if op == "PRINT" {
            args.push(txt)
          } else {
            for part in txt.split(",") {
              args.push(part.trim)
            }
          }
        }
      }
      ins = Ins(line = i + 1, op = op, args = args, dup = dup)
      ents.push(ins)
      if op != "" {
        prog.push(ins)
      }
    }
  }
  errs : List[Str] = []
  for e in ents {
    if e.dup != "" {
      errs.push("line " + e.line.toStr + ": duplicate label " + e.dup)
    } else if e.op != "" {
      m = err(e, labels)
      if m != "" {
        errs.push("line " + e.line.toStr + ": " + m)
      }
    }
  }
  if errs.length > 0 {
    for m in errs {
      sys.screen.print(m)
    }
    sys.screen.print("assembly failed: " + errs.length.toStr + " errors")
    return
  }
  nums : List[Int] = []
  if cut < lines.length {
    rest = lines.slice(cut + 1).join(" ").replace("\r", " ").replace("\t", " ")
    for w in rest.split(" ") {
      x = w.trim
      if x.isInt {
        nums.push(x.toInt)
      }
    }
  }
  run(prog, labels, nums, sys.screen)
}
```

```
Tx = type { date : Str, text : Str, accts : List[Str], amts : List[Int], isVoid : Bool, voided : Bool }
State = type { accts : List[Str], types : Map[Str], bal : Map, txs : List[Tx] }

fmt(n : Int) : Str {
  a = n
  sign = ""
  if n < 0 {
    a = -n
    sign = "-"
  }
  f = a % 100
  fs = f.toStr
  if f < 10 {
    fs = "0" + fs
  }
  sign + (a / 100).toStr + "." + fs
}

isDigits(s : Str) : Bool {
  if s.length == 0 {
    return false
  }
  for i in 0..s.length {
    c = s.at(i)
    if !(c >= "0" && c <= "9") {
      return false
    }
  }
  true
}

dim(y : Int, m : Int) : Int {
  if m == 2 {
    leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0
    if leap {
      return 29
    }
    return 28
  }
  if m == 4 || m == 6 || m == 9 || m == 11 {
    return 30
  }
  31
}

validDate(d : Str) : Bool {
  if d.length != 10 {
    return false
  }
  if d.at(4) != "-" || d.at(7) != "-" {
    return false
  }
  y = d.slice(0, 4)
  m = d.slice(5, 7)
  dd = d.slice(8, 10)
  if !(isDigits(y) && isDigits(m) && isDigits(dd)) {
    return false
  }
  mo = m.toInt
  day = dd.toInt
  if mo < 1 || mo > 12 {
    return false
  }
  day >= 1 && day <= dim(y.toInt, mo)
}

validAmt(s : Str) : Bool {
  t = s
  if t.startsWith("-") {
    t = t.slice(1)
  }
  if t.length < 4 {
    return false
  }
  if t.at(t.length - 3) != "." {
    return false
  }
  isDigits(t.slice(0, t.length - 3)) && isDigits(t.slice(t.length - 2))
}

parseAmt(s : Str) : Int {
  neg = s.startsWith("-")
  t = s
  if neg {
    t = s.slice(1)
  }
  whole = t.slice(0, t.length - 3).toInt
  frac = t.slice(t.length - 2).toInt
  v = whole * 100 + frac
  if neg {
    v = -v
  }
  v
}

txError(st : State, date : Str, names : List[Str], atxt : List[Str]) : Str {
  if !validDate(date) {
    return "error: bad date " + date
  }
  if st.txs.length > 0 && date < st.txs.at(st.txs.length - 1).date {
    return "error: date out of order"
  }
  if names.length < 2 {
    return "error: too few postings"
  }
  for n in names {
    if !st.types.has(n) {
      return "error: no account " + n
    }
  }
  for a in atxt {
    if a.length > 0 && !validAmt(a) {
      return "error: bad amount " + a
    }
  }
  opens = 0
  sum = 0
  for i in 0..atxt.length {
    if atxt.at(i).length == 0 {
      opens = opens + 1
    } else {
      sum = sum + parseAmt(atxt.at(i))
    }
  }
  if opens > 1 {
    return "error: two open amounts"
  }
  if opens == 0 && sum != 0 {
    return "error: off by " + fmt(sum)
  }
  ""
}

addTx(st : State, tx : Tx) : Int !Mut {
  st.txs.push(tx)
  for i in 0..tx.accts.length {
    n = tx.accts.at(i)
    st.bal.set(n, st.bal.get(n) + tx.amts.at(i))
  }
  st.txs.length
}

doTx(st : State, out : Screen, line : Str, posts : List[Str]) !Mut !IO {
  parts = line.split(" ")
  date = ""
  if parts.length > 1 {
    date = parts.at(1)
  }
  text = ""
  if parts.length > 2 {
    text = parts.slice(2).join(" ")
  }
  names : List[Str] = []
  atxt : List[Str] = []
  for p in posts {
    t = p.trim
    sp = t.find(" ")
    if sp < 0 {
      names.push(t)
      atxt.push("")
    } else {
      names.push(t.slice(0, sp))
      atxt.push(t.slice(sp + 1).trim)
    }
  }
  err = txError(st, date, names, atxt)
  if err != "" {
    out.print(err)
  } else {
    amts : List[Int] = []
    sum = 0
    openAt = -1
    for i in 0..atxt.length {
      if atxt.at(i).length == 0 {
        openAt = i
        amts.push(0)
      } else {
        v = parseAmt(atxt.at(i))
        amts.push(v)
        sum = sum + v
      }
    }
    if openAt >= 0 {
      amts.set(openAt, -sum)
    }
    k = addTx(st, Tx(date = date, text = text, accts = names, amts = amts, isVoid = false, voided = false))
    out.print("tx " + k.toStr)
  }
}

doVoid(st : State, out : Screen, parts : List[Str]) !Mut !IO {
  n = 0
  if parts.length > 1 && parts.at(1).isInt {
    n = parts.at(1).toInt
  }
  if n < 1 || n > st.txs.length {
    out.print("error: no tx " + parts.at(1 % parts.length).replace("", ""))
  } else {
    o = st.txs.at(n - 1)
    if o.isVoid {
      out.print("error: tx " + n.toStr + " is a void")
    } else if o.voided {
      out.print("error: tx " + n.toStr + " already void")
    } else {
      amts : List[Int] = []
      for a in o.amts {
        amts.push(-a)
      }
      date = st.txs.at(st.txs.length - 1).date
      k = addTx(st, Tx(date = date, text = "void of " + n.toStr, accts = o.accts, amts = amts, isVoid = true, voided = false))
      o.voided = true
      out.print("tx " + k.toStr)
    }
  }
}

doBalance(st : State, out : Screen, parts : List[Str]) !IO {
  prefix = ""
  if parts.length > 1 {
    prefix = parts.at(1)
  }
  total = 0
  for n in st.accts.sort {
    if prefix.length == 0 || n == prefix || n.startsWith(prefix + ":") {
      b = st.bal.get(n)
      if b != 0 {
        out.print(fmt(b).padLeft(12) + " " + n)
        total = total + b
      }
    }
  }
  out.print("------------")
  out.print(fmt(total).padLeft(12) + " total")
}

lastColon(s : Str) : Int {
  i = s.length - 1
  while i >= 0 {
    if s.at(i) == ":" {
      return i
    }
    i = i - 1
  }
  -1
}

parentOf(s : Str) : Str {
  k = lastColon(s)
  if k < 0 {
    return ""
  }
  s.slice(0, k)
}

nodeSum(st : State, name : Str) : Int {
  t = 0
  for a in st.accts {
    if a == name || a.startsWith(name + ":") {
      t = t + st.bal.get(a)
    }
  }
  t
}

treeRec(st : State, out : Screen, nodes : List[Str], parent : Str, depth : Int) !IO {
  kids : List[Str] = []
  for n in nodes {
    if parentOf(n) == parent {
      kids.push(n)
    }
  }
  indent = ""
  for d in 0..depth {
    indent = indent + "  "
  }
  for n in kids.sort {
    amt = nodeSum(st, n)
    if amt != 0 {
      out.print(fmt(amt).padLeft(12) + "  " + indent + n.slice(lastColon(n) + 1))
    }
    treeRec(st, out, nodes, n, depth + 1)
  }
}

doTree(st : State, out : Screen) !IO {
  nodes : List[Str] = []
  seen = Map()
  for a in st.accts {
    ps = a.split(":")
    cur = ""
    for k in 0..ps.length {
      if k == 0 {
        cur = ps.at(0)
      } else {
        cur = cur + ":" + ps.at(k)
      }
      if !seen.has(cur) {
        seen.set(cur, 1)
        nodes.push(cur)
      }
    }
  }
  treeRec(st, out, nodes, "", 0)
}

doRegister(st : State, out : Screen, parts : List[Str]) !IO {
  name = ""
  if parts.length > 1 {
    name = parts.at(1)
  }
  if !st.types.has(name) {
    out.print("error: no account " + name)
  } else {
    run = 0
    for tx in st.txs {
      for i in 0..tx.accts.length {
        if tx.accts.at(i) == name {
          a = tx.amts.at(i)
          run = run + a
          out.print(tx.date + " " + fmt(a).padLeft(10) + " " + fmt(run).padLeft(10) + " " + tx.text)
        }
      }
    }
  }
}

doMonthly(st : State, out : Screen, parts : List[Str]) !IO {
  year = ""
  if parts.length > 1 {
    year = parts.at(1)
  }
  for m in 1..=12 {
    mm = m.toStr
    if m < 10 {
      mm = "0" + mm
    }
    prefix = year + "-" + mm
    inc = 0
    exp = 0
    has = false
    for tx in st.txs {
      if tx.date.slice(0, 7) == prefix {
        for i in 0..tx.accts.length {
          ty = st.types.get(tx.accts.at(i))
          if ty == "income" {
            inc = inc + tx.amts.at(i)
            has = true
          } else if ty == "expense" {
            exp = exp + tx.amts.at(i)
            has = true
          }
        }
      }
    }
    if has {
      a = -inc
      out.print(prefix + " income " + fmt(a) + " expense " + fmt(exp) + " net " + fmt(a - exp))
    }
  }
}

doCheck(st : State, out : Screen, parts : List[Str]) !IO {
  name = ""
  amt = ""
  if parts.length > 1 {
    name = parts.at(1)
  }
  if parts.length > 2 {
    amt = parts.at(2)
  }
  if !st.types.has(name) {
    out.print("error: no account " + name)
  } else {
    b = fmt(st.bal.get(name))
    if b == amt {
      out.print("ok")
    } else {
      out.print("error: " + name + " is " + b + ", not " + amt)
    }
  }
}

doOpen(st : State, out : Screen, parts : List[Str]) !Mut !IO {
  name = ""
  ty = ""
  if parts.length > 1 {
    name = parts.at(1)
  }
  if parts.length > 2 {
    ty = parts.at(2)
  }
  if !(ty == "asset" || ty == "liability" || ty == "equity" || ty == "income" || ty == "expense") {
    out.print("error: bad type " + ty)
  } else if st.types.has(name) {
    out.print("error: account " + name + " exists")
  } else {
    st.types.set(name, ty)
    st.accts.push(name)
    out.print("opened " + name)
  }
}

main(sys) {
  a : List[Str] = []
  t : List[Tx] = []
  st = State(accts = a, types = Map[Str](), bal = Map(), txs = t)
  lines = sys.stdin.split("\n")
  i = 0
  while i < lines.length {
    line = lines.at(i).replace("\r", "")
    i = i + 1
    tl = line.trim
    if tl.length == 0 {
      continue
    }
    parts = tl.split(" ")
    cmd = parts.at(0)
    if cmd == "open" {
      doOpen(st, sys.screen, parts)
    } else if cmd == "tx" {
      posts : List[Str] = []
      while i < lines.length && lines.at(i).startsWith("  ") {
        pl = lines.at(i).replace("\r", "")
        if pl.trim.length > 0 {
          posts.push(pl)
        }
        i = i + 1
      }
      doTx(st, sys.screen, tl, posts)
    } else if cmd == "void" {
      doVoid(st, sys.screen, parts)
    } else if cmd == "balance" {
      doBalance(st, sys.screen, parts)
    } else if cmd == "tree" {
      doTree(st, sys.screen)
    } else if cmd == "register" {
      doRegister(st, sys.screen, parts)
    } else if cmd == "monthly" {
      doMonthly(st, sys.screen, parts)
    } else if cmd == "check" {
      doCheck(st, sys.screen, parts)
    } else {
      sys.screen.print("error: bad command " + cmd)
    }
  }
}
```
