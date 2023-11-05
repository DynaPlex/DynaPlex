import datetime
import os
import shutil
import sys
import pprint

import tomli

# -- Project information
sys.path.insert(0, os.path.abspath('../../'))

now = datetime.date.today()

project = "DynaPlex"
authors = "DynaPlex contributors"
copyright = f"2023 - {now.year}, {authors}"

with open("../pyproject.toml", "rb") as fh:
    pyproj = tomli.load(fh)

# -- API documentation
autoclass_content = "class"
autodoc_member_order = "bysource"
autodoc_typehints = "signature"

# -- nbsphinx
nbsphinx_execute = "always"

# -- General configuration
extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.napoleon",
    'breathe',
    "sphinx_immaterial",
    "nbsphinx",
]

exclude_patterns = []

breathe_projects = {
    'DynaPlex': '../../docs/doxygen/xml',
}
breathe_default_project = 'DynaPlex'

for project, path in breathe_projects.items():
    full_path = os.path.abspath(os.path.join(os.path.dirname(__file__), path))
    print(f"Breathe project '{project}' path: {full_path}")

source_suffix = ['.rst','.xml']

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "sphinx": ("https://www.sphinx-doc.org/en/master/", None),
}
intersphinx_disabled_domains = ["std"]

add_module_names = False
python_use_unqualified_type_names = True

# -- Options for HTML output
html_theme = "sphinx_immaterial"
html_logo = "assets/images/icon.png"
html_theme_options = {
    "site_url": "https://dynaplex.nl/",
    "repo_url": "https://github.com/WillemvJ/DynaPlexPrivate/",
    "icon": {
        "repo": "fontawesome/brands/github",
        "edit": "material/file-edit-outline",
    },
    "features": [
        "navigation.top",
        "navigation.path",
        "navigation.prune",
        "toc.follow",
        "toc.integrate",
        "navigation.indexes"
    ],
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "primary": "DarkCyan",
            "accent": "CornflowerBlue",
            "scheme": "default",
            "toggle": {
                "icon": "material/lightbulb-outline",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "primary": "DarkCyan",
            "accent": "CornflowerBlue",
            "scheme": "slate",
            "toggle": {
                "icon": "material/lightbulb",
                "name": "Switch to light mode",
            },
        },
    ],
    "version_dropdown": False,
    "version_info": [
        {
            "version": "",
            "title": "v1.0",
            "aliases": [],
        },
    ],
}

# -- Options for EPUB output
epub_show_urls = "footnote"
