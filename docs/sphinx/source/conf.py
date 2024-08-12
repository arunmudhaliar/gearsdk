# Configuration file for the Sphinx documentation builder.
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import sys
sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'gearsdk'
copyright = '2024, amudaliar'
author = 'amudaliar'
release = '0.1'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'sphinx.ext.duration',
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
    'build_time_extension',
    # 'exhale',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',  # Add your Google Analytics ID if needed
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    'style_nav_header_background': '#2980B9',  # Change the header background color
    
    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False,

    # Other options
    'body_max_width': None,  # None or a string like '800px' or '90%'
    'sidebarwidth': '300px',  # Default is 240px
}

html_static_path = ['_static']
html_css_files = [
    'custom.css',
]

# -- Breathe configuration ---------------------------------------------------
breathe_projects = {
    "gearsdk": "../../doxygen-build/xml"
}
breathe_default_project = "gearsdk"

# -- Exhale configuration (optional) -----------------------------------------
# Uncomment to enable Exhale
# exhale_args = {
#     "containmentFolder": "./exhale-api",
#     "rootFileName": "index.rst",
#     "rootFileTitle": "API Documentation",
#     "doxygenStripFromPath": "..",
#     "createTreeView": True,
#     "treeViewIsBootstrap": True,  # If using the sphinx-bootstrap-theme
#     "exhaleExecutesDoxygen": True,  # Make Exhale run Doxygen automatically
#     "exhaleDoxygenStdin": "INPUT = \
#                         ../../../qh3server \
#                         ../../../common \
#                         ../../../networkcommon \
#                         ../../../qclient \
#                         ../../../qh3client \
#                         ../../../qhiredis \
#                         ../../../qserver \
#                         ../../../qstats-crwaler \
#                         ../../../qutils \
#                         ../../../qzookeeper \
#                         ../../../servercommon \
#                         \nEXCLUDE = ../../../common/crypto \
#                         ../../../common/libbson-1.0 \
#                         ../../../common/nvwa \
#                         ../../../common/rapidjson \
#                         ../../../common/uthash \
#                         ../../../common/zlib \
#                         \nEXCLUDE_PATTERNS = */ev/* \
#                         */quiche/* \
#                         */hiredis/* \
#                         */libpgsql/* \
#                         */qpgsql/* \
#                         */dpp/* \
#                         */zookeeper/* \
#                         */libmongoc-1.0/* \
#                         */crypto/* \
#                         */rapidjson/*"
# }

# -- Language configuration ---------------------------------------------------
# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the Pygments highlight language should be.
highlight_language = 'cpp'
