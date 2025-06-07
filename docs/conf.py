# -*- coding: utf-8 -*-
from datetime import datetime

extensions = [
    'm2r2',
    'sphinx_rtd_theme',
    'sphinx.ext.todo',
]

source_suffix = '.rst'

master_doc = 'index'

# General information about the project.
project = 'navit'
year = datetime.now().year
author = 'The Navit Team'
html_theme = "sphinx_rtd_theme"

# Fix Unicode characters in latex
latex_engine = "xelatex"
latex_use_xindy = False
latex_elements = {
    "preamble": "\\usepackage[UTF8]{ctex}\n",
}
