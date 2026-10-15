# A Rouge lexer for colm, which the manual's build loads with "asciidoctor -r"
# to highlight its [source,colm] blocks. Rouge has no lexer for colm. The
# tokens follow colm's own grammar, src/colm/colm.lm.

# Rouge is optional. Without it, Asciidoctor warns that it is missing and
# renders the blocks plain, so there is nothing to define.
begin
	require 'rouge'
rescue LoadError
	return
end

require 'set'

module Rouge
	module Lexers
		class Colm < RegexLexer
			title 'Colm'
			desc 'Colm, the COmputer Language Machinery'
			tag 'colm'
			filenames '*.lm'

			def self.keywords
				@keywords ||= Set.new %w(
					if elsif else while for in switch case default return
					break yield reject print prints parse parse_tree parse_stop
					reduce read_reduce construct cons match require send
					send_tree new cast typeid make_token make_tree commit ni
					preeof eos
				)
			end

			def self.declarations
				@declarations ||= Set.new %w(
					def redef lex end token rl ignore literal reduction
					namespace struct alias context global export iter include
					reducefirst left right nonassoc
				)
			end

			def self.types
				@types ||= Set.new %w(
					int bool void list list_el map map_el ptr ref parser accum
					str any stream
				)
			end

			def self.builtins
				@builtins ||= Set.new %w(
					argv arg0 stdin stdout stderr error input match_text
					match_length exit exit_hard atoi atoo open system tolower
					toupper prefix suffix postfix xml xmlac
					child rev_child repeat rev_repeat triter with_ignore
					list_iter rev_list_iter map_iter sprintf
				)
			end

			# The machines a regular expression can use without defining.
			def self.machines
				@machines ||= Set.new %w(
					any ascii extend alpha digit alnum lower upper cntrl graph
					print punct space xdigit null zlen empty
				)
			end

			# The C and C++ words a reduction's code is likely to use.
			def self.host_types
				@host_types ||= Set.new %w(
					unsigned signed void char short int long float double bool
					const static struct class auto
				)
			end

			def self.host_keywords
				@host_keywords ||= Set.new %w(
					if else for while do return break continue switch case
					default new delete this sizeof
				)
			end

			id = /[a-zA-Z_][a-zA-Z0-9_]*/

			# A word that starts something else ends a definition with no
			# expression, such as "token NAME -".
			stop = /(?=(?:#{(declarations + keywords).to_a.join('|')})\b)/

			state :whitespace do
				rule %r/\s+/m, Text
				rule %r/#.*$/, Comment::Single
			end

			state :root do
				mixin :whitespace

				# A slash opens a regular expression only in the definition of
				# a token, an ignore or an rl. Anywhere else it divides.
				rule %r/(token|ignore|rl)\b/, Keyword::Declaration, :tokdef
				rule %r/reduction\b/, Keyword::Declaration, :reduction

				rule %r/"/, Str::Double, :dq
				rule %r/'/, Str::Single, :sq
				rule %r/`[^\s][^\s`\]]*`?/, Str::Symbol
				rule %r/~.*$/, Str::Other

				rule %r/\d+/, Num::Integer

				# A production's name, after its brackets.
				rule %r/(\])(\s*)(:)(#{id})/ do
					groups Punctuation, Text, Punctuation, Name::Function
				end

				# The name in a declaration, such as "X: int".
				rule %r/(#{id})(\s*)(:)(?!:)/ do
					groups Name::Variable, Text, Punctuation
				end

				rule id do |m|
					w = m[0]
					if %w(nil true false).include?(w)
						token Keyword::Constant
					elsif self.class.keywords.include?(w)
						token Keyword
					elsif self.class.declarations.include?(w)
						token Keyword::Declaration
					elsif self.class.types.include?(w)
						token Keyword::Type
					elsif self.class.builtins.include?(w)
						token Name::Builtin
					else
						token Name
					end
				end

				rule %r/::|->|<-|<<|==|!=|<=|>=|&&|\|\||\.\.\./, Operator
				rule %r([-+*/%=<>!^@$|.?:&]), Operator
				rule %r/[\[\](){},;]/, Punctuation
				rule %r/./, Text
			end

			# After "token", "ignore" or "rl": the name, any attributes, and
			# the "ni-" that keeps the ignores off, up to the slash that opens
			# the expression. Anything else, as in "token NAME -", ends it.
			state :tokdef do
				mixin :whitespace
				rule %r/ni\b/, Keyword
				rule(stop) { pop! }
				rule %r/(#{id})(\s*)(:)(?!:)/ do
					groups Name::Variable, Text, Punctuation
				end
				rule id do |m|
					if self.class.types.include?(m[0])
						token Keyword::Type
					else
						token Name::Class
					end
				end
				rule %r/-/, Operator
				rule %r(/), Str::Regex, :regex
				rule(//) { pop! }
			end

			state :regex do
				rule %r(/) do
					token Str::Regex
					pop! 2
				end
				mixin :whitespace
				rule %r/'(\\.|[^'\\])*'i?/, Str::Single
				rule %r/"(\\.|[^"\\])*"i?/, Str::Double
				rule %r/\[\^?(\\.|[^\]\\])*\]/, Str::Regex
				rule %r/0x[0-9a-fA-F]+|\d+/, Num
				rule id do |m|
					if self.class.machines.include?(m[0])
						token Name::Builtin
					else
						token Name
					end
				end
				rule %r/\*\*|\.\.|--|:>>|:>|<:|[-.|&*?+^()]/, Operator
				rule %r/./, Str::Regex
			end

			state :dq do
				rule %r/"|\n/, Str::Double, :pop!
				rule %r/\\./m, Str::Escape
				rule %r/\[/, Str::Interpol, :embedded
				rule %r/[^"\[\\\n]+/, Str::Double
			end

			state :sq do
				rule %r/'|\n/, Str::Single, :pop!
				rule %r/\\./m, Str::Escape
				rule %r/[^'\\\n]+/, Str::Single
			end

			# The code, or the pattern, in a "[ ... ]" inside a string. It can
			# have brackets of its own.
			state :embedded do
				rule %r/\[/, Punctuation, :nested
				rule %r/\]/, Str::Interpol, :pop!
				mixin :root
			end

			state :nested do
				rule %r/\[/, Punctuation, :nested
				rule %r/\]/, Punctuation, :pop!
				mixin :root
			end

			state :reduction do
				mixin :whitespace
				rule %r/end\b/, Keyword::Declaration, :pop!
				rule %r/\{/, Punctuation, :host
				rule %r/(:)(\s*)(#{id})/ do
					groups Punctuation, Text, Name::Function
				end
				rule %r/::/, Operator
				rule id, Name::Class
				rule %r/./, Text
			end

			# The C or C++ code of a reduction, with its references to the
			# production's elements.
			state :host do
				rule %r/\{/, Punctuation, :host
				rule %r/\}/, Punctuation, :pop!
				rule %r/\$\$|\$\*?(#{id}|\d+)|@(#{id}|\d+)/, Name::Variable
				rule %r(//.*$), Comment::Single
				rule %r(/\*.*?\*/)m, Comment::Multiline
				rule %r/'(\\.|[^'\\])*'/, Str::Char
				rule %r/"(\\.|[^"\\])*"/, Str::Double
				rule %r/\d+/, Num
				rule id do |m|
					if self.class.host_types.include?(m[0])
						token Keyword::Type
					elsif self.class.host_keywords.include?(m[0])
						token Keyword
					else
						token Name
					end
				end
				rule %r/\s+/m, Text
				rule %r/->|[-+*\/%=<>!&|^~?:.]/, Operator
				rule %r/./, Punctuation
			end
		end
	end
end
