# convert_gtest_xml_to_rst.py
import xml.etree.ElementTree as ET
import argparse

def parse_gtest_xml(input_file, output_file, title):
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Start the RST content
    rst_content = f'{title}\n{"=" * len(title)}\n\n'

    for testcase in root.iter('testcase'):
        name = testcase.get('name')
        classname = testcase.get('classname')
        result = 'Passed'
        message = ''
        
        # Check if the testcase has failures
        for failure in testcase.iter('failure'):
            result = 'Failed'
            message = failure.text
            rst_content += f'**Test Case:** {classname}::{name}\n\n**Result:** {result}\n\n**Message:** {message}\n\n'
        
        # If no failure, it's passed
        if result == 'Passed':
            rst_content += f'**Test Case:** {classname}::{name}\n\n**Result:** {result}\n\n'

    # Write the RST content to the output file
    with open(output_file, 'w') as f:
        f.write(rst_content)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert Google Test XML to reStructuredText (RST).')
    parser.add_argument('input_file', type=str, help='Path to the input XML file')
    parser.add_argument('output_file', type=str, help='Path to the output RST file')
    parser.add_argument('title', type=str, help='Title for RST file')
    args = parser.parse_args()

    parse_gtest_xml(args.input_file, args.output_file, args.title)
