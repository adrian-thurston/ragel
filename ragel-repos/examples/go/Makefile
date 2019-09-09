ragel = ragel

check: atoi rpn url
	./atoi
	./rpn
	./url
	@echo PASS

graph: atoi.dot rpn.dot url.dot url_authority.dot
	xdot atoi.dot
	xdot rpn.dot
	xdot url.dot
	xdot url_authority.dot

atoi: atoi.go
atoi.go: atoi.rl
atoi.dot: atoi.rl

rpn: rpn.go
rpn.go: rpn.rl
rpn.dot: rpn.rl

url: url.go url_authority.go
url.go: url.rl
url.dot: url.rl
url_authority.go: url_authority.rl
url_authority.dot: url_authority.rl

clean:       ; rm -f *.go *.dot atoi rpn url
%: %.go      ; go build -o $@ $^
%.go: %.rl   ; $(ragel) -Z -T0 -o $@ $<
%.dot: %.rl  ; $(ragel) -V -Z -p -o $@ $<
