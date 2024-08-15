import xml.etree.ElementTree as ET
import argparse

def format_message(message, max_width=60):
    """Format the message to fit within the max_width for table cells."""
    words = message.split()
    lines = []
    current_line = ""
    for word in words:
        if len(current_line) + len(word) + 1 > max_width:
            lines.append(current_line)
            current_line = word
        else:
            if current_line:
                current_line += " "
            current_line += word
    if current_line:
        lines.append(current_line)
    return '\n       '.join(lines)

def parse_gtest_xml(input_file, output_file, title):
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Start the RST content with the title and list-table directive
    rst_content = f'{title}\n{"=" * len(title)}\n\n'
    rst_content += (".. raw:: html\n\n"
                    "   <table class=\"test-result-table\">\n"
                    "   <thead>\n"
                    "   <tr>\n"
                    "     <th>Test Case</th>\n"
                    "     <th>Result</th>\n"
                    "     <th>Message</th>\n"
                    "   </tr>\n"
                    "   </thead>\n"
                    "   <tbody>\n")

    row_count = 0

    for testcase in root.iter('testcase'):
        name = testcase.get('name')
        classname = testcase.get('classname')
        result = 'Passed'
        message = ''
        
        for failure in testcase.iter('failure'):
            result = 'Failed'
            message = failure.text.strip()  # Strip any extra newlines or spaces
            
            # Format the message for the list-table
            formatted_message = format_message(message, max_width=60)
            
            # Add the row to the HTML table with a class for failed results
            row_class = 'failed' if result == 'Failed' else 'passed'
            rst_content += (f"   <tr class=\"{row_class}\">\n"
                           f"     <td>{classname}::{name}</td>\n"
                           f"     <td>{result}</td>\n"
                           f"     <td>{formatted_message}</td>\n"
                           "   </tr>\n")
            row_count += 1
        
        if result == 'Passed':
            # Add passed test case row to the list-table
            rst_content += (f"   <tr class=\"passed\">\n"
                           f"     <td>{classname}::{name}</td>\n"
                           f"     <td>{result}</td>\n"
                           f"     <td>No issues</td>\n"
                           "   </tr>\n")
            row_count += 1

    rst_content += "   </tbody>\n   </table>\n"

    # Write the RST content to the output file
    with open(output_file, 'w') as f:
        f.write(rst_content)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert Google Test XML to reStructuredText (RST) with HTML table format.')
    parser.add_argument('input_file', type=str, help='Path to the input XML file')
    parser.add_argument('output_file', type=str, help='Path to the output RST file')
    parser.add_argument('title', type=str, help='Title for RST file')
    args = parser.parse_args()

    parse_gtest_xml(args.input_file, args.output_file, args.title)
