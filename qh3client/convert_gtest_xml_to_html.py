# convert_gtest_xml_to_html.py
import xml.etree.ElementTree as ET
import argparse

def parse_gtest_xml(input_file, output_file, title):
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Add the HTML header including the stylesheet link
    html_content = f'''
    <html>
    <head>
        <link rel="stylesheet" href="../_static/styles.css">
    </head>
    <body>
    <h1>{title}</h1>
    '''

    for testcase in root.iter('testcase'):
        name = testcase.get('name')
        classname = testcase.get('classname')
        result = 'Passed'
        message = ''
        
        for failure in testcase.iter('failure'):
            result = 'Failed'
            message = failure.text
            html_content += f'<p><b>Test Case:</b> {classname}::{name}<br><b>Result:</b> {result}<br><b>Message:</b> {message}</p>'
        
        if result == 'Passed':
            html_content += f'<p><b>Test Case:</b> {classname}::{name}<br><b>Result:</b> {result}</p>'

    html_content += '''
    </body>
    </html>
    '''

    with open(output_file, 'w') as f:
        f.write(html_content)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert Google Test XML to HTML.')
    parser.add_argument('input_file', type=str, help='Path to the input XML file')
    parser.add_argument('output_file', type=str, help='Path to the output HTML file')
    parser.add_argument('title', type=str, help='Title for HTML file')
    args = parser.parse_args()

    parse_gtest_xml(args.input_file, args.output_file, args.title)
