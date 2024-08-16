import xml.etree.ElementTree as ET
import argparse

def format_message(message, max_width=60):
    """Format the message to fit within the max_width for table cells and convert newlines to HTML <br>."""
    lines = []
    for original_line in message.splitlines():
        words = original_line.split()
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
    
    # Join the lines with <br> for HTML
    return '<br>'.join(lines)

def parse_gtest_xml(input_file, output_file, title):
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Capture testsuites summary information
    total_tests = root.get('tests')
    total_failures = root.get('failures')
    total_disabled = root.get('disabled')
    total_errors = root.get('errors')
    total_time = root.get('time')
    timestamp = root.get('timestamp')

    # Start the RST content with the title and link to the CSS file
    rst_content = f'{title}\n{"=" * len(title)}\n\n'
    rst_content += (".. raw:: html\n\n"
                    "   <link rel=\"stylesheet\" type=\"text/css\" href=\"../_static/custom.css\">\n"
                    "   <h3>Summary</h3>\n"
                    "   <table class=\"summary-table\">\n"
                    "   <tr>\n"
                    f"     <td><strong>Total tests:</strong> {total_tests}</td>\n"
                    f"     <td><strong>Failures:</strong> {total_failures}</td>\n"
                    f"     <td><strong>Disabled:</strong> {total_disabled}</td>\n"
                    f"     <td><strong>Errors:</strong> {total_errors}</td>\n"
                    f"     <td><strong>Total time taken:</strong> {total_time}s</td>\n"
                    f"     <td><strong>Timestamp:</strong> {timestamp}</td>\n"
                    "   </tr>\n"
                    "   </table>\n\n"
                    "   <div class=\"double-line-separator\"></div>\n\n")

    # Loop through each testsuite
    for idx, testsuite in enumerate(root.iter('testsuite')):
        suite_name = testsuite.get('name')
        suite_tests = testsuite.get('tests')
        suite_failures = testsuite.get('failures')
        suite_disabled = testsuite.get('disabled')
        suite_errors = testsuite.get('errors')
        suite_time = testsuite.get('time')
        suite_timestamp = testsuite.get('timestamp')

        # Determine the CSS class for disabled test suites
        suite_class = 'disabled' if suite_disabled != '0' else 'normal'

        # Add a summary for the current test suite using HTML
        rst_content += (".. raw:: html\n\n"
                        f"   <h3 class=\"{suite_class}\">Suite: {suite_name}</h3>\n"
                        "   <table class=\"suite-summary-table\">\n"
                        "   <tr>\n"
                        f"     <td><strong>Tests:</strong> {suite_tests}</td>\n"
                        f"     <td><strong>Failures:</strong> {suite_failures}</td>\n"
                        f"     <td><strong>Disabled:</strong> {suite_disabled}</td>\n"
                        f"     <td><strong>Errors:</strong> {suite_errors}</td>\n"
                        f"     <td><strong>Time:</strong> {suite_time}s</td>\n"
                        f"     <td><strong>Timestamp:</strong> {suite_timestamp}</td>\n"
                        "   </tr>\n"
                        "   </table>\n\n"
                        "   <table class=\"test-result-table\">\n"
                        "   <thead>\n"
                        "   <tr>\n"
                        "     <th>Test Case</th>\n"
                        "     <th>Result</th>\n"
                        "     <th>Message</th>\n"
                        "     <th>Execution time</th>\n"
                        "     <th>Timestamp</th>\n"
                        "   </tr>\n"
                        "   </thead>\n"
                        "   <tbody>\n")

        # Loop through each testcase in the testsuite
        for testcase in testsuite.iter('testcase'):
            name = testcase.get('name')
            classname = testcase.get('classname')
            result = 'Passed'
            message = 'No issues'
            time = testcase.get('time')
            test_timestamp = testcase.get('timestamp')

            # Check if the test case is disabled
            status = testcase.get('status')
            result = 'Disabled' if status == 'notrun' else 'Passed'
            row_class = 'disabled' if result == 'Disabled' else 'passed'

            for failure in testcase.iter('failure'):
                result = 'Failed'
                message = failure.text.strip()  # Strip any extra newlines or spaces
                
                # Format the message for the table cell
                formatted_message = format_message(message, max_width=60)

                # Add the row to the HTML table with a class for failed results
                row_class = 'failed' if result == 'Failed' else 'passed'
                rst_content += (f"   <tr class=\"{row_class}\">\n"
                                f"     <td>{classname}::{name}</td>\n"
                                f"     <td>{result}</td>\n"
                                f"     <td>{formatted_message}</td>\n"
                                f"     <td>{time}s</td>\n"
                                f"     <td>{test_timestamp}</td>\n"
                                "   </tr>\n")

            if result == 'Passed':
                # Add passed test case row to the table
                rst_content += (f"   <tr class=\"passed\">\n"
                                f"     <td>{classname}::{name}</td>\n"
                                f"     <td>{result}</td>\n"
                                f"     <td>{message}</td>\n"
                                f"     <td>{time}s</td>\n"
                                f"     <td>{test_timestamp}</td>\n"
                                "   </tr>\n")

            if result == 'Disabled':
                # Add disabled test case row to the table with grey background
                rst_content += (f"   <tr class=\"disabled\">\n"
                                f"     <td>{classname}::{name}</td>\n"
                                f"     <td>{result}</td>\n"
                                f"     <td>{message}</td>\n"
                                f"     <td>{time}s</td>\n"
                                f"     <td>{test_timestamp}</td>\n"
                                "   </tr>\n")

        # Close the HTML table for the current test suite
        rst_content += "   </tbody>\n   </table>\n\n"

        # Add a single line separator after each test suite except the last one
        if idx < len(root.findall('testsuite')) - 1:
            rst_content += (".. raw:: html\n\n"
                            "   <div class=\"single-line-separator\"></div>\n\n")

    # Write the RST content to the output file
    with open(output_file, 'w') as f:
        f.write(rst_content)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert Google Test XML to reStructuredText (RST) with styled summary tables and separators.')
    parser.add_argument('input_file', type=str, help='Path to the input XML file')
    parser.add_argument('output_file', type=str, help='Path to the output RST file')
    parser.add_argument('title', type=str, help='Title for RST file')
    args = parser.parse_args()

    parse_gtest_xml(args.input_file, args.output_file, args.title)
