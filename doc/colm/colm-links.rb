# Checks the links in the colm manual, which the manual's build loads with
# "asciidoctor -r". The manual is one book, and only the book is built, so a
# link in it either leaves the book, with a scheme such as https:, or goes to
# an id in it. Asciidoctor renders a link to an included chapter that names
# no id as href="#", the top of the book, keeps a link to an id that doesn't
# exist, and renders a link to a page the book doesn't include as that page's
# HTML, which isn't built. It reports none of these. An id given twice it only
# warns of, and links to it go to the first. This checks the rendered book for
# all four and logs each as an error. The build passes --failure-level=ERROR,
# so the book then fails to build.

class ColmLinkCheck < Asciidoctor::Extensions::Postprocessor
	include Asciidoctor::Logging

	def process doc, output
		# Only ids inside tags count. An example's code that shows id="..."
		# has its < escaped.
		ids = Hash.new 0
		output.scan(/<[A-Za-z][^<>]*?\sid="([^"]*)"/) { ids[$1] += 1 }
		ids.each do |id, count|
			next if count == 1
			logger.error %(id "#{id}" is given #{count} times in the book, ) +
				%(and links to it go to the first)
		end

		output.scan(/<a\s[^>]*?\bhref="([^"]*)"[^>]*>(.*?)<\/a>/m) do
			href = $1
			# The link's text, as a writer can grep for it: without its tags,
			# and with its character references decoded. Asciidoctor loads
			# cgi/util.
			text = (CGI.unescapeHTML $2.gsub(/<[^>]*>/, '')).gsub(/\s+/, ' ').strip
			if href == '#'
				logger.error %(link "#{text}" names no id, and leads to the ) +
					%(top of the book: name the id above the chapter's title)
			elsif href.start_with? '#'
				next if ids.key? href[1..]
				logger.error %(link "#{text}" goes to #{href}, ) +
					%(which is not an id in the book)
			elsif href !~ /\A[A-Za-z][A-Za-z0-9+.-]*:/
				logger.error %(link "#{text}" goes to #{href}, ) +
					%(which is not in the book)
			end
		end

		output
	end
end

Asciidoctor::Extensions.register do
	postprocessor ColmLinkCheck
end
