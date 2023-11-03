import datetime
import os
import shutil
import sys

import tomli

# -- Project information
sys.path.insert(0, os.path.abspath("../../"))

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

# -- numpydoc
numpydoc_xref_param_type = True
numpydoc_class_members_toctree = False
numpydoc_attributes_as_param_list = False
napoleon_include_special_with_doc = True

# -- nbsphinx
nbsphinx_execute = "always"

# -- General configuration
extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.napoleon",
    "sphinx_immaterial",
    "nbsphinx",
    "numpydoc",
]

exclude_patterns = ["_build", "**.ipynb_checkpoints"]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "sphinx": ("https://www.sphinx-doc.org/en/master/", None),
}
intersphinx_disabled_domains = ["std"]

templates_path = ["_templates"]

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

python_resolve_unqualified_typing = True
python_transform_type_annotations_pep585 = True
python_transform_type_annotations_pep604 = True
object_description_options = [
    ("py:.*", dict(include_fields_in_toc=False, include_rubrics_in_toc=False)),
    ("py:attribute", dict(include_in_toc=False)),
    ("py:parameter", dict(include_in_toc=False)),
]


# -- Options for EPUB output
epub_show_urls = "footnote"
