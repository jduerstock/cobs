# $+ means "The names of all the prerequisites, with spaces between them, exactly as given"
# $@ means "The file name of the target of the rule"
# For more magic automatic variables, see
# <http://www.gnu.org/manual/make-3.80/html_chapter/make_10.html#SEC111>

example: example.c COBSEncoder.c
	cc $+ -o $@
