from docutils import nodes
from docutils.parsers.rst import Directive
from sphinx.util.docutils import SphinxDirective
from datetime import datetime

class BuildTimeDirective(SphinxDirective):
    has_content = False

    def run(self):
        build_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        p = nodes.paragraph(text=f"Document generated on: {build_time}")
        p['classes'] += ['build-time']  # Add the CSS class
        return [p]

def setup(app):
    app.add_directive('build_time', BuildTimeDirective)
