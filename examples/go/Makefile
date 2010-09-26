
check: atoi rpn url
	./atoi
	./rpn
	./url
	@echo PASS

graph: atoi.png rpn.png url.png url_authority.png
	evince atoi.png
	evince rpn.png
	evince url.png
	evince url_authority.png

atoi: atoi.6
atoi.6: atoi.go
atoi.go: atoi.rl
atoi.dot: atoi.rl
atoi.png: atoi.dot

rpn: rpn.6
rpn.6: rpn.go
rpn.go: rpn.rl
rpn.dot: rpn.rl
rpn.png: rpn.dot

url: url.6
url.6: url.go url_authority.go
url.go: url.rl
url.dot: url.rl
url.png: url.dot
url_authority.go: url_authority.rl
url_authority.dot: url_authority.rl
url_authority.png: url_authority.dot

clean:       ; rm -f *.6 *.go *.png *.dot atoi rpn url
%: %.6       ; 6l -o $@ $^
%.6: %.go    ; 6g -o $@ $^
%.go: %.rl   ; ragel -Z -G2 -o $@ $<
%.dot: %.rl  ; ragel -V -p -Z -G2 -o $@ $<
%.png: %.dot ; dot -Tpng -o $@ $<
