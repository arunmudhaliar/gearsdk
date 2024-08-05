# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

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
    # 'exhale',
    ]

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# html_theme = 'classic'
# html_theme = 'sphinx_book_theme'
# html_theme = 'bootstrap'
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    
    'logo_only': False,

    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_static_path = ['_static']

# Configure Breathe
breathe_projects = {
    "gearsdk": "../../doxygen/build/xml"
}
breathe_default_project = "gearsdk"


# # Exhale Configuration
# exhale_args = {
#     # These arguments are required
#     "containmentFolder": "./exhale-api",
#     "rootFileName": "index.rst",
#     "rootFileTitle": "API Documentation",
#     "doxygenStripFromPath": "..",
#     # Suggested optional arguments
#     "createTreeView": True,
#     # TIP: if using the sphinx-bootstrap-theme, you need
#     "treeViewIsBootstrap": True,
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
#                         ,
# }

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'