import re
import os
import sys

# Define regex patterns for function names, variable names, and constants
snake_case_pattern = re.compile(r'^[a-z][a-z0-9_]*$')
constant_pattern = re.compile(
    r'\b(?:static\s+const\s+|const\s+)(?:int|float|double|char|bool|std::string|unsigned\s+int|long\s+int|short\s+int|long\s+double|.*)\s+([A-Z][A-Z0-9_]*)\s*(?:[=;])'
)
variable_declaration_pattern = re.compile(
    r'\b(?:int|float|double|char|bool|std::string|unsigned\s+int|long\s+int|short\s+int|long\s+double|std::\s*[a-zA-Z_][a-zA-Z0-9_]*|[a-zA-Z_][a-zA-Z0-9_]*<[^>]*>)\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*=\s*[^,;]*)?(?:\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*=\s*[^,;]*)?)*(?:\s*;\s*)?$'
)

def process_line(line, inside_block_comment):
    """
    Process a line to remove comments and documentation while keeping line numbers.
    Returns the line with comments removed and the status of whether the line was code or comment.
    """
    # Remove inline comments
    line = re.sub(r'//.*$', '', line)

    if inside_block_comment:
        # Handle block comment continuation
        if '*/' in line:
            inside_block_comment = False
            line = line.split('*/', 1)[1]
        else:
            # Skip the entire line if inside a block comment
            return '', inside_block_comment

    # Process block comments
    block_comment_start = line.find('/*')
    while block_comment_start != -1:
        block_comment_end = line.find('*/', block_comment_start + 2)
        if block_comment_end == -1:
            inside_block_comment = True
            line = line[:block_comment_start]
            break
        else:
            line = line[:block_comment_start] + line[block_comment_end + 2:]
            block_comment_start = line.find('/*')

    # Remove documentation comments if not inside block comment
    if not inside_block_comment:
        doc_comment_start = line.find('/**')
        if doc_comment_start != -1:
            doc_comment_end = line.find('*/', doc_comment_start + 3)
            if doc_comment_end != -1:
                line = line[:doc_comment_start] + line[doc_comment_end + 2:]
            else:
                line = line[:doc_comment_start]
    
    processed_line = line.strip()
    is_code = bool(processed_line)  # Check if the line is non-empty after processing
    return processed_line, inside_block_comment

def check_function_names(file_path):
    print(f"Checking functions -> {file_path}")
    has_error = False
    inside_block_comment = False
    with open(file_path, 'r') as file:
        lines = file.readlines()

    for line_num, line in enumerate(lines, start=1):
        processed_line, inside_block_comment = process_line(line, inside_block_comment)
        if processed_line:
            # Check for function declarations and definitions
            if re.search(r'\b(?:void|int|float|double|char|bool|std::string|.*)\s+[a-zA-Z_]\w*\s*\(.*\)\s*(?:\{|\;)', processed_line):
                function_name = re.search(r'\b[a-zA-Z_]\w*\b(?=\()', processed_line)
                if function_name:
                    name = function_name.group()
                    if not snake_case_pattern.fullmatch(name) or '__' in name:
                        # Format for Xcode clickable link: file:line:column
                        print(f"{file_path}:{line_num}:0: error: Naming convention violation: {name}")
                        has_error = True
    return has_error

def check_variable_names(file_path):
    print(f"Checking variables -> {file_path}")
    has_error = False
    inside_block_comment = False
    with open(file_path, 'r') as file:
        lines = file.readlines()

    for line_num, line in enumerate(lines, start=1):
        processed_line, inside_block_comment = process_line(line, inside_block_comment)
        if processed_line:
            # Check for constants
            constant_matches = constant_pattern.findall(processed_line)
            for const in constant_matches:
                if not re.fullmatch(r'[A-Z][A-Z0-9_]*', const):
                    # Format for Xcode clickable link: file:line:column
                    print(f"{file_path}:{line_num}:0: error: Constant naming convention violation: {const}")
                    has_error = True

            # Check for variable declarations
            variable_matches = variable_declaration_pattern.findall(processed_line)
            for var_tuple in variable_matches:
                for var in var_tuple:
                    # Skip empty variables (e.g., empty tuples from `findall`)
                    if not var:
                        continue
                    
                    # Check if the variable name is in snake_case
                    if not snake_case_pattern.fullmatch(var) or '__' in var:
                        # Check if it's a valid variable name
                        if re.fullmatch(r'[a-zA-Z_][a-zA-Z0-9_]*', var) and not re.fullmatch(r'[A-Z][A-Z0-9_]*', var):
                            # Format for Xcode clickable link: file:line:column
                            print(f"{file_path}:{line_num}:0: error: Variable naming convention violation: {var}")
                            has_error = True

    return has_error

def main(directory, include_dirs, exclude_paths):
    print(f"Checking naming conventions in {directory}")
    function_check_error = False
    variable_check_error = False
    for root, _, files in os.walk(directory):
        # Check if the current root directory is in include_dirs
        if include_dirs and not any(root.startswith(inc_dir) for inc_dir in include_dirs):
            continue
        
        # Skip ignored directories and files
        if any(ignored_path in root for ignored_path in exclude_paths):
            continue
        
        for file in files:
            if file.endswith(('.cpp', '.c', '.h', '.hpp')):
                file_path = os.path.join(root, file)
                # Skip ignored files
                if any(ignored_file in file_path for ignored_file in exclude_paths):
                    print(f"Skipping {file_path}")
                    continue
                
                if check_function_names(file_path):
                    function_check_error = True
                if check_variable_names(file_path):
                    variable_check_error = True
    
    if function_check_error or variable_check_error:
        sys.exit(1)  # Exit with status code 1 to indicate errors

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script.py <directory> [include_dirs...] -- [exclude_paths...]")
        sys.exit(1)
    
    directory = sys.argv[1]
    include_dirs = []
    exclude_paths = []

    # Parse include directories and exclude paths
    parsing_includes = True
    for arg in sys.argv[2:]:
        if arg == '--':
            parsing_includes = False
            continue
        
        if parsing_includes:
            include_dirs.append(arg)
        else:
            exclude_paths.append(arg)
    
    main(directory, include_dirs, exclude_paths)
