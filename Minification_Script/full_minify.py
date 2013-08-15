from slimit import minify
import cssmin
import html5lib

from bs4 import BeautifulSoup

def read_in_file():
	with open('../SD Web Files/gui.htm', 'rU') as full_file:
		soup = BeautifulSoup(full_file.read(), "html5lib")
		# JS
		scripts = soup("script", {"id":"script_parse"})
		script_text = scripts[0].string
		mini_script = minify(script_text, mangle=True, mangle_toplevel=True)
		scripts[0].string = mini_script
		# CSS
		css = soup("style", {"type":"text/css"})
		css_text = css[0].string
		mini_css = cssmin.cssmin(css_text)
		css[0].string = mini_css
		# HTML
		# Too tired. Just select all the HTML and remove indents.
		# Write to file
		with open('../SD Web Files/gui.min.htm', 'w') as mini_file:
			mini_file.write(str(soup))
		# mini_file = open('SD Web Files/gui.min.htm', 'w')
		# regex = re.compile("<script type=\"text/javascript\">(.+)</script>",re.MULTILINE|re.DOTALL)
		# content = full_file.read()
		# text = regex.findall(content)
		# print text[0]
		# mini_file.write(minify(scripts[0], mangle=True, mangle_toplevel=True))

if __name__ == '__main__':
	read_in_file()