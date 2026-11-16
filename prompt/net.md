## Simpler: the web and mail

This adds to the one page; all of its rules stay.

- The web: `sys.net.fetch(url)` gives the body, `sys.net.fetchWith(url, ["X-Key: abc"])` the same
  with headers, `sys.net.post(url, body)` sends; all stop the
  program on failure, so ask first with `sys.net.reach(url)` (the status, 0 if nothing answers) when a
  miss is expected. The top names each
  host, without its port: `fetches "api.github.com", "127.0.0.1"`; a url made while running is checked against it. The parameter type is `Net`.
- Mail: `sys.mail.send(to, subject, body)`, with a top line that says to whom: `mails "me@example.com"`
  (`"*@example.com"` allows everyone there). Its parameter type is `Mail`, and the function says `!IO`.
